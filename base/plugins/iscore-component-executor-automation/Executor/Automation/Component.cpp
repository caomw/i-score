#include <ossia/editor/automation/automation.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <QDebug>
#include <QString>
#include <algorithm>
#include <vector>

#include "Component.hpp"
#include <Curve/CurveModel.hpp>
#include <Curve/Segment/CurveSegmentData.hpp>
#include <Device/Protocol/DeviceInterface.hpp>
#include <Device/Protocol/DeviceList.hpp>
#include <Device/Protocol/DeviceSettings.hpp>

#include <ossia/editor/curve/curve.hpp>
#include <ossia/editor/curve/curve_segment.hpp>
#include <ossia/editor/value/value.hpp>
#include <ossia/network/base/address.hpp>
#include <ossia/network/base/node.hpp>
#include <Engine/Protocols/OSSIADevice.hpp>
#include <Engine/Executor/ProcessElement.hpp>
#include <State/Address.hpp>
#include <Engine/CurveConversion.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <iscore/plugins/customfactory/StringFactoryKey.hpp>
#include <Engine/iscore2OSSIA.hpp>

#include <Engine/Executor/ExecutorContext.hpp>
#include <Engine/Executor/DocumentPlugin.hpp>

namespace Automation
{
namespace RecreateOnPlay
{
Component::Component(
        ::Engine::Execution::ConstraintElement& parentConstraint,
        ::Automation::ProcessModel& element,
        const ::Engine::Execution::Context& ctx,
        const Id<iscore::Component>& id,
        QObject *parent):
    ::Engine::Execution::ProcessComponent_T<Automation::ProcessModel, ossia::automation>{
          parentConstraint, element, ctx, id, "Executor::Automation::Component", parent},
    m_deviceList{ctx.devices.list()}
{
    recreate();
}

void Component::recreate()
{
    m_ossia_curve.reset(); // It will be remade after.
    m_ossia_process = nullptr;

    auto address = Engine::iscore_to_ossia::findAddress(
          m_deviceList,
          process().address());

    if(address)
    {
        m_addressType = address->getValueType();

      if(process().tween())
          on_curveChanged(ossia::Destination(*address, {})); // If the type changes we need to rebuild the curve.
      else
          on_curveChanged({});

      if(m_ossia_curve)
      {
        m_ossia_process = new ossia::automation(
                    *address,
                    ossia::Behavior(m_ossia_curve));
      }
    }
}

template<typename Y_T>
std::shared_ptr<ossia::curve_abstract> Component::on_curveChanged_impl(
        const optional<ossia::Destination>& d)
{
    using namespace ossia;

    const double min = process().min();
    const double max = process().max();

    auto scale_x = [] (double val) -> double { return val; };
    auto scale_y = [=] (double val) -> Y_T { return val * (max - min) + min; };

    auto segt_data = process().curve().sortedSegments();
    if(segt_data.size() != 0)
    {
        return Engine::iscore_to_ossia::curve<double, Y_T>(scale_x, scale_y, segt_data, d);
    }
    else
    {
        return {};
    }
}

std::shared_ptr<ossia::curve_abstract> Component::on_curveChanged(
        const optional<ossia::Destination>& d)
{
    m_ossia_curve.reset();
    switch(m_addressType)
    {
        case ossia::val_type::INT:
            m_ossia_curve = on_curveChanged_impl<int>(d);
            break;
        case ossia::val_type::FLOAT:
            m_ossia_curve = on_curveChanged_impl<float>(d);
            break;
        default:
            qDebug() << "Unsupported curve type: " << (int)m_addressType;
            ISCORE_TODO;
    }

    return m_ossia_curve;
}
}
}
