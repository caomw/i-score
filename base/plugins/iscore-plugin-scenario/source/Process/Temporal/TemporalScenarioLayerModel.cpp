#include "TemporalScenarioLayerModel.hpp"

#include "Document/Constraint/Rack/Slot/SlotModel.hpp"
#include "Document/Constraint/ConstraintModel.hpp"
#include "Process/ScenarioModel.hpp"
#include "Document/Constraint/ViewModels/Temporal/TemporalConstraintViewModel.hpp"

#include <ProcessInterface/LayerModelPanelProxy.hpp>
#include "StateMachines/ScenarioStateMachine.hpp"
#include "TemporalScenarioPresenter.hpp"
#include "TemporalScenarioPanelProxy.hpp"

TemporalScenarioLayerModel::TemporalScenarioLayerModel(
        const id_type<LayerModel>& viewModelId,
        const QMap<id_type<ConstraintModel>, id_type<ConstraintViewModel> >& constraintIds,
        ScenarioModel& model,
        QObject* parent) :
    AbstractScenarioLayer {viewModelId,
                              "TemporalScenarioLayer",
                              model,
                              parent}
{
    for(auto& key : constraintIds.keys())
    {
        makeConstraintViewModel(key, constraintIds.value(key));
    }
}

TemporalScenarioLayerModel::TemporalScenarioLayerModel(
        const TemporalScenarioLayerModel& source,
        const id_type<LayerModel>& id,
        ScenarioModel& newScenario,
        QObject* parent) :
    AbstractScenarioLayer {source,
                              id,
                              "TemporalScenarioLayer",
                              newScenario,
                              parent
}
{
    for(TemporalConstraintViewModel* src_constraint : constraintsViewModels(source))
    {
        // TODO some room for optimization here
        addConstraintViewModel(
                    src_constraint->clone(
                        src_constraint->id(),
                        newScenario.constraint(src_constraint->model().id()),
                        this));
    }
}

LayerModelPanelProxy*TemporalScenarioLayerModel::make_panelProxy(QObject* parent) const
{
    return new TemporalScenarioPanelProxy{*this, parent};
}


void TemporalScenarioLayerModel::makeConstraintViewModel(
        const id_type<ConstraintModel>& constraintModelId,
        const id_type<ConstraintViewModel>& constraintViewModelId)
{
    auto& constraint_model = model(*this).constraint(constraintModelId);

    auto constraint_view_model =
        constraint_model.makeConstraintViewModel<constraint_layer_type> (
            constraintViewModelId,
            this);

    addConstraintViewModel(constraint_view_model);
}

void TemporalScenarioLayerModel::addConstraintViewModel(constraint_layer_type* constraint_view_model)
{
    m_constraints.push_back(constraint_view_model);

    emit constraintViewModelCreated(constraint_view_model->id());
}

void TemporalScenarioLayerModel::on_constraintRemoved(const id_type<ConstraintModel>& constraintSharedModelId)
{
    for(auto& constraint_view_model : constraintsViewModels(*this))
    {
        if(constraint_view_model->model().id() == constraintSharedModelId)
        {
            removeConstraintViewModel(constraint_view_model->id());
            return;
        }
    }
}