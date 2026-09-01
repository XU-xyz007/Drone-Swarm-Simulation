#include "DdsService.h"

#include <windows.h>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>

#include "ZRDDSCppSimpleInterface.h"
#include "../SharedMessage/gen_code/DroneMessageDataReader.h"
#include "../SharedMessage/gen_code/DroneMessageDataWriter.h"
#include "../SharedMessage/gen_code/DroneMessageTypeSupport.h"

using namespace DDS;

namespace {

std::mutex gMutex;
DomainParticipant* gParticipant = NULL;
SharedMessage::DroneCmdDataWriter* gCommandWriter = NULL;
SharedMessage::DroneStateDataWriter* gStateWriter = NULL;
DataReader* gCommandReader = NULL;
DataReader* gStateReader = NULL;
SwarmComm::CommandCallback gCommandCallback;
SwarmComm::StateCallback gStateCallback;

std::string ExecutableDirectory()
{
    char path[MAX_PATH] = {};
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) return std::string();
    char* slash = strrchr(path, '\\');
    if (slash == NULL) return std::string();
    *slash = '\0';
    return std::string(path);
}

class CommandReaderListener : public SimpleDataReaderListener<SharedMessage::DroneCmd, SharedMessage::DroneCmdSeq, SharedMessage::DroneCmdDataReader> {
public:
    void on_process_sample(DataReader*, const SharedMessage::DroneCmd& sample, const SampleInfo&) override
    {
        SwarmComm::CommandCallback callback;
        { std::lock_guard<std::mutex> lock(gMutex); callback = gCommandCallback; }
        if (callback) callback(sample);
    }
};

class StateReaderListener : public SimpleDataReaderListener<SharedMessage::DroneState, SharedMessage::DroneStateSeq, SharedMessage::DroneStateDataReader> {
public:
    void on_process_sample(DataReader*, const SharedMessage::DroneState& sample, const SampleInfo&) override
    {
        SwarmComm::StateCallback callback;
        { std::lock_guard<std::mutex> lock(gMutex); callback = gStateCallback; }
        if (callback) callback(sample);
    }
};

} // namespace

namespace SwarmComm {

const char* const kCommandTopic = "Topic_Drone_Command";
const char* const kStateTopic = "Topic_Drone_State";
const int kDomainId = 150;

bool InitDDS()
{
    std::lock_guard<std::mutex> lock(gMutex);
    if (gParticipant != NULL) return true;
    // ZR-DDS searches the current directory for zrddslicence.lic.  Set it to
    // the executable directory so users may launch from any PowerShell path.
    const std::string executableDirectory = ExecutableDirectory();
    if (!executableDirectory.empty()) SetCurrentDirectoryA(executableDirectory.c_str());
    const std::string qosPath = executableDirectory.empty()
        ? "ZRDDS_QOS_PROFILES.xml"
        : executableDirectory + "\\ZRDDS_QOS_PROFILES.xml";
    if (DDSIF::Init(qosPath.c_str(), "non_rio") == NULL) return false;
    gParticipant = DDSIF::CreateDP(kDomainId, "udp_dp");
    if (gParticipant == NULL) { DDSIF::Finalize(); return false; }

    gCommandWriter = static_cast<SharedMessage::DroneCmdDataWriter*>(DDSIF::PubTopic(
        gParticipant, kCommandTopic, SharedMessage::DroneCmdTypeSupport::get_instance(), "command_reliable", NULL));
    gStateWriter = static_cast<SharedMessage::DroneStateDataWriter*>(DDSIF::PubTopic(
        gParticipant, kStateTopic, SharedMessage::DroneStateTypeSupport::get_instance(), "state_best_effort", NULL));
    // DDS owns the listener after SubTopic succeeds (it is attached to the
    // DataReader).  Keep temporary ownership until that handoff succeeds so a
    // failed subscription cannot leak a listener.
    std::unique_ptr<CommandReaderListener> commandListener(new CommandReaderListener());
    gCommandReader = DDSIF::SubTopic(gParticipant, kCommandTopic,
        SharedMessage::DroneCmdTypeSupport::get_instance(), "command_reliable", commandListener.get());
    if (gCommandReader != NULL) commandListener.release();

    std::unique_ptr<StateReaderListener> stateListener(new StateReaderListener());
    gStateReader = DDSIF::SubTopic(gParticipant, kStateTopic,
        SharedMessage::DroneStateTypeSupport::get_instance(), "state_best_effort", stateListener.get());
    if (gStateReader != NULL) stateListener.release();

    if (gCommandWriter != NULL && gStateWriter != NULL && gCommandReader != NULL && gStateReader != NULL) return true;
    if (gCommandReader != NULL) DDSIF::UnSubTopic(gCommandReader);
    if (gStateReader != NULL) DDSIF::UnSubTopic(gStateReader);
    if (gCommandWriter != NULL) DDSIF::UnPubTopic(gCommandWriter);
    if (gStateWriter != NULL) DDSIF::UnPubTopic(gStateWriter);
    // DDSIF::Finalize is the ZR-DDS simple-interface lifecycle endpoint: it
    // releases the participant after all of its readers/writers are removed.
    DDSIF::Finalize();
    gParticipant = NULL; gCommandWriter = NULL; gStateWriter = NULL; gCommandReader = NULL; gStateReader = NULL;
    return false;
}

bool PublishCmd(const SharedMessage::DroneCmd& cmd)
{
    std::lock_guard<std::mutex> lock(gMutex);
    return gCommandWriter != NULL && gCommandWriter->write(cmd, HANDLE_NIL_NATIVE) == RETCODE_OK;
}

bool PublishState(const SharedMessage::DroneState& state)
{
    std::lock_guard<std::mutex> lock(gMutex);
    return gStateWriter != NULL && gStateWriter->write(state, HANDLE_NIL_NATIVE) == RETCODE_OK;
}

bool SubscribeCmd(CommandCallback onCommand)
{
    std::lock_guard<std::mutex> lock(gMutex);
    gCommandCallback = onCommand;
    return static_cast<bool>(gCommandCallback);
}

bool SubscribeState(StateCallback onState)
{
    std::lock_guard<std::mutex> lock(gMutex);
    gStateCallback = onState;
    return static_cast<bool>(gStateCallback);
}

void ShutdownDDS()
{
    std::lock_guard<std::mutex> lock(gMutex);
    if (gCommandReader != NULL) DDSIF::UnSubTopic(gCommandReader);
    if (gStateReader != NULL) DDSIF::UnSubTopic(gStateReader);
    if (gCommandWriter != NULL) DDSIF::UnPubTopic(gCommandWriter);
    if (gStateWriter != NULL) DDSIF::UnPubTopic(gStateWriter);
    gCommandCallback = CommandCallback();
    gStateCallback = StateCallback();
    // There is no DDSIF::DeleteDP in the ZR-DDS simple interface.  Finalize
    // is the documented owner-level cleanup and releases gParticipant here.
    DDSIF::Finalize();
    gParticipant = NULL; gCommandWriter = NULL; gStateWriter = NULL; gCommandReader = NULL; gStateReader = NULL;
}

} // namespace SwarmComm
