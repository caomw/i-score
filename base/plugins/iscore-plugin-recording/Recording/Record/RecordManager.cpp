#include <Automation/AutomationModel.hpp>
#include <Automation/Commands/InitAutomation.hpp>
#include <Curve/Segment/PointArray/PointArraySegment.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Explorer/Explorer/DeviceExplorerModel.hpp>
#include <Scenario/Commands/Constraint/AddOnlyProcessToConstraint.hpp>
#include <Scenario/Commands/Constraint/AddRackToConstraint.hpp>
#include <Scenario/Commands/Constraint/Rack/AddSlotToRack.hpp>
#include <Scenario/Commands/Scenario/Creations/CreateConstraint_State_Event_TimeNode.hpp>
#include <Scenario/Commands/Scenario/Creations/CreateTimeNode_Event_State.hpp>
#include <Scenario/Commands/Scenario/Displacement/MoveNewEvent.hpp>
#include <Scenario/Commands/Scenario/ShowRackInViewModel.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <core/document/Document.hpp>

#include <QApplication>
#include <qnamespace.h>
#include <QString>
#include <algorithm>
#include <type_traits>
#include <utility>

#include <Automation/AutomationProcessMetadata.hpp>
#include <Recording/Commands/Record.hpp>
#include <Curve/CurveModel.hpp>
#include <Device/Address/AddressSettings.hpp>
#include <Device/Address/IOType.hpp>
#include <Device/Node/DeviceNode.hpp>
#include <Device/Protocol/DeviceInterface.hpp>
#include <Device/Protocol/DeviceList.hpp>

#include <Explorer/Explorer/ListeningManager.hpp>
#include <Process/ExpandMode.hpp>
#include <Process/Process.hpp>
#include <Process/TimeValue.hpp>
#include <Recording/Record/RecordData.hpp>
#include <Recording/Record/RecordManager.hpp>
#include <Scenario/Commands/Constraint/Rack/Slot/AddLayerModelToSlot.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/Constraint/Rack/RackModel.hpp>
#include <Scenario/Document/Constraint/Rack/Slot/SlotModel.hpp>
#include <Scenario/Palette/ScenarioPoint.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <State/Value.hpp>
#include <State/ValueConversion.hpp>
#include <iscore/command/Dispatchers/MacroCommandDispatcher.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <iscore/tools/ModelPath.hpp>
#include <iscore/tools/NotifyingMap.hpp>
#include <iscore/tools/SettableIdentifier.hpp>
#include <iscore/tools/TreeNode.hpp>
#include <Curve/Settings/CurveSettingsModel.hpp>
namespace Curve
{
class SegmentModel;
}

namespace Recording
{
AutomationRecorder::AutomationRecorder(
        RecordContext& ctx):
    context{ctx},
    m_settings{context.context.app.settings<Curve::Settings::Model>()}
{
}

void AutomationRecorder::stop()
{
    // Stop all the recording machinery
    auto msecs = context.time();
    for(const auto& con : m_recordCallbackConnections)
    {
        QObject::disconnect(con);
    }
    m_recordCallbackConnections.clear();

    qApp->processEvents();

    // Record and then stop
    if(!context.started())
    {
        context.dispatcher.rollback();
        return;
    }

    auto simplify = m_settings.getSimplify();
    auto simplifyRatio = m_settings.getSimplificationRatio();
    // Add a last point corresponding to the current state

    // Create commands for the state of each automation to send on
    // the network, and push them silently.

    // Potentially simplify curve and transform it in segments
    for(const auto& recorded : records)
    {
        Curve::PointArraySegment& segt = recorded.second.segment;

        auto& automation = *safe_cast<Automation::ProcessModel*>(
                    recorded.second.curveModel.parent());

        // Here we add a last point equal to the latest recorded point
        {
            // Add last point
            segt.addPoint(msecs.msec(), segt.points().rbegin()->second);

            automation.setDuration(msecs);
        }

        // Conversion of the piecewise to segments, and
        // serialization.
        if(simplify)
            recorded.second.segment.simplify(simplifyRatio);
        // TODO if there is no remaining segment or an invalid segment, don't add it.

        // Add a point with the last state.
        auto initCurveCmd = new Automation::InitAutomation{
                automation,
                recorded.first,
                recorded.second.segment.min(),
                recorded.second.segment.max(),
                recorded.second.segment.toPowerSegments()};

        // This one shall not be redone
        context.dispatcher.submitCommand(recorded.second.addProcCmd);
        context.dispatcher.submitCommand(recorded.second.addLayCmd);
        context.dispatcher.submitCommand(initCurveCmd);
    }
}

void AutomationRecorder::messageCallback(const State::Address &addr, const State::Value &val)
{
    using namespace std::chrono;
    if(context.started())
    {
        auto msecs = context.time();

        auto newval = State::convert::value<float>(val.val);

        auto it = records.find(addr);
        ISCORE_ASSERT(it != records.end());

        const auto& proc_data = it->second;

        proc_data.segment.addPoint(msecs.msec(), newval);

        static_cast<Automation::ProcessModel*>(proc_data.curveModel.parent())->setDuration(msecs);
    }
    else
    {
        emit firstMessageReceived();
        context.start();

        auto newval = State::convert::value<float>(val.val);

        auto it = records.find(addr);
        ISCORE_ASSERT(it != records.end());

        const auto& proc_data = it->second;
        proc_data.segment.addPoint(0, newval);
    }
}

void AutomationRecorder::parameterCallback(const State::Address &addr, const State::Value &val)
{
    using namespace std::chrono;
    if(context.started())
    {
        auto msecs = context.time();

        auto newval = State::convert::value<float>(val.val);

        auto it = records.find(addr);
        ISCORE_ASSERT(it != records.end());

        const auto& proc_data = it->second;

        auto last = proc_data.segment.points().rbegin();
        proc_data.segment.addPoint(msecs.msec() - 1, last->second);
        proc_data.segment.addPoint(msecs.msec(), newval);

        static_cast<Automation::ProcessModel*>(proc_data.curveModel.parent())->setDuration(msecs);
    }
    else
    {
        emit firstMessageReceived();
        context.start();

        auto newval = State::convert::value<float>(val.val);

        auto it = records.find(addr);
        ISCORE_ASSERT(it != records.end());

        const auto& proc_data = it->second;
        proc_data.segment.addPoint(0, newval);
    }
}

bool AutomationRecorder::setup(const Box& box, const RecordListening& recordListening)
{
    std::vector<std::vector<State::Address>> addresses;
    //// Creation of the curves ////
    for(const auto& vec : recordListening)
    {
        addresses.push_back({Device::address(*vec.front())});
        addresses.back().reserve(vec.size());

        for(auto node : vec)
        {
            auto& addr = node->get<Device::AddressSettings>();
            // Note : since we directly create the IDs here, we don't have to worry
            // about their generation.
            auto cmd_proc = new Scenario::Command::AddOnlyProcessToConstraint{
                    Path<Scenario::ConstraintModel>(box.constraint),
                    Metadata<ConcreteFactoryKey_k, Automation::ProcessModel>::get()};
            cmd_proc->redo();
            auto& proc = box.constraint.processes.at(cmd_proc->processId());
            auto& autom = static_cast<Automation::ProcessModel&>(proc);


            auto cmd_layer = new Scenario::Command::AddLayerModelToSlot{
                             box.slot, proc};
            cmd_layer->redo();

            autom.curve().clear();

            auto val = State::convert::value<float>(addr.value);
            auto min = State::convert::value<float>(addr.domain.min);
            auto max = State::convert::value<float>(addr.domain.max);

            Curve::SegmentData seg;
            seg.id = Id<Curve::SegmentModel>{0};
            seg.start = {0, val};
            seg.end = {1, -1};
            seg.specificSegmentData =
                    QVariant::fromValue(
                        Curve::PointArraySegmentData{ 0, 1, min, max, { {0, val} } });
            auto segt = new Curve::PointArraySegment{
                        seg,
                    &autom.curve()};

            segt->setStart({0, val});
            segt->setEnd({1, -1});
            segt->addPoint(0, val);

            autom.curve().addSegment(segt);

            addresses.back().push_back(Device::address(*node));
            records.insert(
                        std::make_pair(
                            addresses.back().back(),
                            RecordData{
                                cmd_proc,
                                cmd_layer,
                                autom.curve(),
                                *segt}));
        }
    }

    const auto& devicelist = context.explorer.deviceModel().list();
    //// Setup listening on the curves ////
    auto callback_to_use =
            m_settings.getCurveMode() == Curve::Settings::Mode::Parameter
            ? &AutomationRecorder::parameterCallback
            : &AutomationRecorder::messageCallback;

    int i = 0;
    for(const auto& vec : recordListening)
    {
        auto& dev = devicelist.device(*vec.front());
        if(!dev.connected())
            continue;

        dev.addToListening(addresses[i]);
        // Add a custom callback.
        m_recordCallbackConnections.push_back(
                    connect(&dev, &Device::DeviceInterface::valueUpdated,
                this, callback_to_use));

        i++;
    }

    return true;
}

Priority AutomationRecorderFactory::matches(
        const Device::Node& n,
        const iscore::DocumentContext& ctx)
{
    return 2;

}

std::unique_ptr<RecordProvider> AutomationRecorderFactory::make(
        const Device::NodeList&,
        const iscore::DocumentContext& ctx)
{
    return {};

}

}
