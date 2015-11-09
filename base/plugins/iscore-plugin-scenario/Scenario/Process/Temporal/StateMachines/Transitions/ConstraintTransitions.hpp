#pragma once
#include <Scenario/Process/Temporal/StateMachines/ScenarioStateMachineBaseTransitions.hpp>


template<>
class ScenarioTransition_T<ScenarioElement::Constraint + Modifier::Click_tag::value> final :
        public MatchedScenarioTransition<PositionedWithId_ScenarioEvent<ConstraintModel, ScenarioElement::Constraint + Modifier::Click_tag::value>>
{
    public:
        using MatchedScenarioTransition::MatchedScenarioTransition;

    protected:
        void onTransition(QEvent * ev) override;
};
using ClickOnConstraint_Transition = ScenarioTransition_T<ScenarioElement::Constraint + Modifier::Click_tag::value>;


template<>
class ScenarioTransition_T<ScenarioElement::Constraint + Modifier::Move_tag::value> final :
        public MatchedScenarioTransition<PositionedWithId_ScenarioEvent<ConstraintModel, ScenarioElement::Constraint + Modifier::Move_tag::value>>
{
    public:
        using MatchedScenarioTransition::MatchedScenarioTransition;

    protected:
        void onTransition(QEvent * ev) override;
};
using MoveOnConstraint_Transition = ScenarioTransition_T<ScenarioElement::Constraint + Modifier::Move_tag::value>;