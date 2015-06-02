#include "SelectionToolState.hpp"
#include <QKeyEventTransition>
#include "Process/ScenarioGlobalCommandManager.hpp"
#include "Process/Temporal/StateMachines/ScenarioStateMachine.hpp"
#include "Process/Temporal/StateMachines/ScenarioStateMachineBaseTransitions.hpp"


#include "Process/ScenarioModel.hpp"
#include "Process/Temporal/TemporalScenarioPresenter.hpp"
#include "Process/Temporal/TemporalScenarioView.hpp"

#include <iscore/document/DocumentInterface.hpp>

#include "Document/Event/EventModel.hpp"
#include "Document/Event/EventPresenter.hpp"
#include "Document/Event/EventView.hpp"

#include "Document/TimeNode/TimeNodeModel.hpp"
#include "Document/TimeNode/TimeNodePresenter.hpp"
#include "Document/TimeNode/TimeNodeView.hpp"

#include "Document/Constraint/ConstraintModel.hpp"
#include "Document/Constraint/ViewModels/AbstractConstraintView.hpp"
#include "Document/Constraint/ViewModels/Temporal/TemporalConstraintPresenter.hpp"

#include <QKeyEventTransition>
#include <QFinalState>
#include <QGraphicsScene>

class CommonSelectionState : public QState
{
    private:
        QState* m_singleSelection{};
        QState* m_multiSelection{};
        QState* m_waitState{};


    public:
        iscore::SelectionDispatcher dispatcher;

        CommonSelectionState(
                iscore::SelectionStack& stack,
                QGraphicsObject* process_view,
                QState* parent):
            QState{parent},
            dispatcher{stack}
        {
            setChildMode(QState::ChildMode::ParallelStates);
            setObjectName("metaSelectionState");
            {
                // Multi-selection state
                auto selectionModeState = new QState{this};
                selectionModeState->setObjectName("selectionModeState");
                {
                    m_singleSelection = new QState{selectionModeState};

                    selectionModeState->setInitialState(m_singleSelection);
                    m_multiSelection = new QState{selectionModeState};

                    auto trans1 = new QKeyEventTransition(process_view,
                                                          QEvent::KeyPress, Qt::Key_Control, m_singleSelection);
                    trans1->setTargetState(m_multiSelection);
                    auto trans2 = new QKeyEventTransition(process_view,
                                                          QEvent::KeyRelease, Qt::Key_Control, m_multiSelection);
                    trans2->setTargetState(m_singleSelection);
                }


                /// Proper selection stuff
                auto selectionState = new QState{this};
                selectionState->setObjectName("selectionState");
                {
                    // Wait
                    m_waitState = new QState{selectionState};
                    m_waitState->setObjectName("m_waitState");
                    selectionState->setInitialState(m_waitState);

                    // Area
                    auto selectionAreaState = new QState{selectionState};
                    selectionAreaState->setObjectName("selectionAreaState");

                    make_transition<Press_Transition>(m_waitState, selectionAreaState);
                    selectionAreaState->addTransition(selectionAreaState, SIGNAL(finished()), m_waitState);
                    {
                        // States
                        auto pressAreaSelection = new QState{selectionAreaState};
                        pressAreaSelection->setObjectName("pressAreaSelection");
                        selectionAreaState->setInitialState(pressAreaSelection);
                        auto moveAreaSelection = new QState{selectionAreaState};
                        moveAreaSelection->setObjectName("moveAreaSelection");
                        auto releaseAreaSelection = new QFinalState{selectionAreaState};
                        releaseAreaSelection->setObjectName("releaseAreaSelection");

                        // Transitions
                        make_transition<Move_Transition>(pressAreaSelection, moveAreaSelection);
                        make_transition<Release_Transition>(pressAreaSelection, releaseAreaSelection);

                        make_transition<Move_Transition>(moveAreaSelection, moveAreaSelection);
                        make_transition<Release_Transition>(moveAreaSelection, releaseAreaSelection);

                        // Operations
                        connect(pressAreaSelection, &QState::entered,
                                this, &CommonSelectionState::on_pressAreaSelection);
                        connect(moveAreaSelection, &QState::entered,
                                this, &CommonSelectionState::on_moveAreaSelection);

                        connect(releaseAreaSelection, &QState::entered,
                                this, &CommonSelectionState::on_releaseAreaSelection);
                    }

                    // Deselection
                    auto deselectState = new QState{selectionState};
                    deselectState->setObjectName("deselectState");
                    make_transition<Cancel_Transition>(selectionAreaState, deselectState);
                    make_transition<Cancel_Transition>(m_waitState, deselectState);
                    deselectState->addTransition(m_waitState);
                    connect(deselectState, &QAbstractState::entered,
                            this, &CommonSelectionState::on_deselect);

                    // Actions on selected elements
                    auto t_delete = new QKeyEventTransition(
                                process_view, QEvent::KeyPress, Qt::Key_Delete, m_waitState);
                    connect(t_delete, &QAbstractTransition::triggered,
                            this, &CommonSelectionState::on_delete);

                    auto t_deleteContent = new QKeyEventTransition(
                                process_view, QEvent::KeyPress, Qt::Key_Backspace, m_waitState);
                    connect(t_deleteContent, &QAbstractTransition::triggered,
                            this, &CommonSelectionState::on_deleteContent);
                }
            }
        }

        virtual void on_pressAreaSelection() = 0;
        virtual void on_moveAreaSelection() = 0;
        virtual void on_releaseAreaSelection() = 0;
        virtual void on_deselect() = 0;
        virtual void on_delete() = 0;
        virtual void on_deleteContent() = 0;

        bool multiSelection() const
        {
            return m_multiSelection->active();
        }
};


template<typename T>
Selection filterSelections(T* pressedModel,
                           Selection sel,
                           bool cumulation)
{
    if(!cumulation)
    {
        sel.clear();
    }

    // If the pressed element is selected
    if(pressedModel->selection.get())
    {
        if(cumulation)
        {
            sel.removeAll(pressedModel);
        }
        else
        {
            sel.push_back(pressedModel);
        }
    }
    else
    {
        sel.push_back(pressedModel);
    }

    return sel;
}



Selection filterSelections(const Selection& newSelection,
                           const Selection& currentSelection,
                           bool cumulation)
{
    return cumulation ? (newSelection + currentSelection).toSet().toList() : newSelection;
}



class ScenarioSelectionState : public CommonSelectionState
{
    private:
        QPointF m_initialPoint;
        QPointF m_movePoint;
        const ScenarioStateMachine& m_parentSM;
        TemporalScenarioView& m_scenarioView;

    public:
        ScenarioSelectionState(
                iscore::SelectionStack& stack,
                const ScenarioStateMachine& parentSM,
                TemporalScenarioView& scenarioview,
                QState* parent):
            CommonSelectionState{stack, &scenarioview, parent},
            m_parentSM{parentSM},
            m_scenarioView{scenarioview}
        {

        }

        const QPointF& initialPoint() const
        { return m_initialPoint; }
        const QPointF& movePoint() const
        { return m_movePoint; }

        void on_pressAreaSelection() override
        {
            m_initialPoint = m_parentSM.scenePoint;
        }

        void on_moveAreaSelection() override
        {
            m_movePoint = m_parentSM.scenePoint;
            m_scenarioView.setSelectionArea(
                        QRectF{m_scenarioView.mapFromScene(m_initialPoint),
                               m_scenarioView.mapFromScene(m_movePoint)}.normalized());
            setSelectionArea(QRectF{m_initialPoint, m_movePoint}.normalized());
        }

        void on_releaseAreaSelection() override
        {
            m_scenarioView.setSelectionArea(QRectF{});
        }

        void on_deselect() override
        {
            dispatcher.setAndCommit(Selection{});
            m_scenarioView.setSelectionArea(QRectF{});
        }

        void on_delete() override
        {
            ScenarioGlobalCommandManager mgr{m_parentSM.commandStack()};
            mgr.deleteSelection(m_parentSM.model());
        }

        void on_deleteContent() override
        {
            ScenarioGlobalCommandManager mgr{m_parentSM.commandStack()};
            mgr.clearContentFromSelection(m_parentSM.model());
        }

        void setSelectionArea(const QRectF& area)
        {
            using namespace std;
            QPainterPath path;
            path.addRect(area);
            Selection sel;
            const auto& events = m_parentSM.presenter().events();
            const auto& timenodes = m_parentSM.presenter().timeNodes();
            const auto& cstrs = m_parentSM.presenter().constraints();

            const auto items = m_parentSM.scene().items(path);


            for (const auto& item : items)
            {
                auto ev_it = find_if(events.cbegin(),
                                     events.cend(),
                                     [&] (EventPresenter* p) { return p->view() == item; });
                if(ev_it != events.cend())
                {
                    sel.push_back(&(*ev_it)->model());
                }
            }

            for (const auto& item : items)
            {
                auto tn_it = find_if(timenodes.cbegin(),
                                     timenodes.cend(),
                                     [&] (TimeNodePresenter* p) { return p->view() == item; });
                if(tn_it != timenodes.cend())
                {
                    sel.push_back(&(*tn_it)->model());
                }
            }

            for (const auto& item : items)
            {
                auto cst_it = find_if(cstrs.begin(),
                                      cstrs.end(),
                                      [&] (AbstractConstraintPresenter* p) { return p->view() == item; });
                if(cst_it != cstrs.end())
                {
                    sel.push_back(&(*cst_it)->model());
                }
            }

            dispatcher.setAndCommit(filterSelections(sel,
                                                       m_parentSM.model().selectedChildren(),
                                                       multiSelection()));
        }
};

SelectionTool::SelectionTool(const ScenarioStateMachine& sm):
    ScenarioToolState{sm}
{
    auto& scenario_view = m_parentSM.presenter().view();

    m_state = new ScenarioSelectionState{
            iscore::IDocument::documentFromObject(m_parentSM.model())->selectionStack(),
            m_parentSM,
            scenario_view,
            &localSM()};
    localSM().setInitialState(m_state);
}




void SelectionTool::on_pressed()
{
    using namespace std;
    mapTopItem(itemUnderMouse(m_parentSM.scenePoint),
    [&] (const id_type<EventModel>& id) // Event
    {
        const auto& elts = m_parentSM.presenter().events();
        auto elt = find_if(begin(elts), end(elts),
                           [&] (EventPresenter* e) { return e->id() == id;});
        Q_ASSERT(elt != end(elts));

        m_state->dispatcher.setAndCommit(filterSelections(&(*elt)->model(),
                                                   m_parentSM.model().selectedChildren(),
                                                   m_state->multiSelection()));
    },
    [&] (const id_type<TimeNodeModel>& id) // TimeNode
    {
        const auto& elts = m_parentSM.presenter().timeNodes();
        auto elt = find_if(begin(elts), end(elts),
                           [&] (TimeNodePresenter* e) { return e->id() == id;});
        Q_ASSERT(elt != end(elts));

        m_state->dispatcher.setAndCommit(filterSelections(&(*elt)->model(),
                                                   m_parentSM.model().selectedChildren(),
                                                   m_state->multiSelection()));
    },
    [&] (const id_type<ConstraintModel>& id) // Constraint
    {
        const auto& elts = m_parentSM.presenter().constraints();
        auto elt = find_if(begin(elts), end(elts),
                           [&] (AbstractConstraintPresenter* e) { return e->id() == id;});
        Q_ASSERT(elt != end(elts));

        m_state->dispatcher.setAndCommit(filterSelections(&(*elt)->model(),
                                                   m_parentSM.model().selectedChildren(),
                                                   m_state->multiSelection()));
    },
    [&] () { localSM().postEvent(new Press_Event); });
}

void SelectionTool::on_moved()
{
    localSM().postEvent(new Move_Event);
    /*
    mapTopItem(itemUnderMouse(m_parentSM.scenePoint),
    [&] (const id_type<EventModel>&) {  },
    [&] (const id_type<TimeNodeModel>&) { localSM().postEvent(new Move_Event); },
    [&] (const id_type<ConstraintModel>&) { localSM().postEvent(new Move_Event); },
    [&] () { localSM().postEvent(new Move_Event); });
    */
}

void SelectionTool::on_released()
{
    localSM().postEvent(new Release_Event);
    /*
    mapTopItem(itemUnderMouse(m_parentSM.scenePoint),
    [&] (const id_type<EventModel>&) {  },
    [&] (const id_type<TimeNodeModel>&) { localSM().postEvent(new Release_Event); },
    [&] (const id_type<ConstraintModel>&) { localSM().postEvent(new Release_Event); },
    [&] () { localSM().postEvent(new Release_Event); });
    */
}

