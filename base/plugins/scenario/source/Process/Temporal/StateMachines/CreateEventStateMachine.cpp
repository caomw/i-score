#include "CreateEventStateMachine.hpp"
#include <Process/ScenarioModel.hpp>
#include <Document/TimeNode/TimeNodeModel.hpp>
#include <Document/Constraint/ConstraintModel.hpp>
#include <Document/Event/EventModel.hpp>

#include "Commands/Scenario/Creations/CreateEventAfterEvent.hpp"
#include <QFinalState>
class PressedState : public QState
{
    public:
        using QState::QState;
};
class MovingState : public QState
{
    public:
        using QState::QState;
};
class ReleasedState : public QState
{
    public:
        using QState::QState;
};

CreateEventState::CreateEventState(ObjectPath &&scenarioPath, iscore::CommandStack& stack, QState* parent):
    CommonState{std::move(scenarioPath), parent},
    m_dispatcher{stack, nullptr}
{
    using namespace Scenario::Command;
    auto finalState = new QFinalState{this};
    QState* mainState = new QState{this};
    {
        QState* pressedState = new PressedState{mainState};
        QState* movingState = new MovingState{mainState};
        QState* releasedState = new ReleasedState{mainState};

        auto tr1 = new ReleaseOnNothing_Transition{*this};
        tr1->setTargetState(releasedState);
        pressedState->addTransition(tr1);
        auto tr2 = new ReleaseOnNothing_Transition{*this};
        tr2->setTargetState(releasedState);
        movingState->addTransition(tr2);

        auto tr3 = new MoveOnNothing_Transition{*this};
        tr3->setTargetState(movingState);
        pressedState->addTransition(tr3);
        auto tr4 = new MoveOnNothing_Transition{*this};
        tr4->setTargetState(movingState);
        movingState->addTransition(tr4);

        releasedState->addTransition(finalState);

        mainState->setInitialState(pressedState);

        QObject::connect(pressedState, &QState::entered, [&] ()
        {
            auto init = new CreateEventAfterEvent{
                                ObjectPath{m_scenarioPath},
                                clickedEvent,
                                date,
                                ypos};
            m_createdEvent = init->createdEvent();
            m_createdTimeNode = init->createdTimeNode();

            m_dispatcher.submitCommand(init);
        });

        QObject::connect(movingState, &QState::entered, [&] ()
        {
            m_dispatcher.submitCommand(
                        new MoveEvent{
                                ObjectPath{m_scenarioPath},
                                m_createdEvent,
                                date,
                                ypos});
        });

        QObject::connect(releasedState, &QState::entered, [&] ()
        {
            m_dispatcher.commit();
        });
    }

    QState* rollbackState = new QState{this};
    mainState->addTransition(this, SIGNAL(cancel()), rollbackState);
    rollbackState->addTransition(finalState);
    QObject::connect(rollbackState, &QState::entered, [&] ()
    {
        m_dispatcher.rollback();
    });

    setInitialState(mainState);
}

