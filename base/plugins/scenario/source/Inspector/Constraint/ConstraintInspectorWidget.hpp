#pragma once

#include <InspectorInterface/InspectorWidgetBase.hpp>
#include <unordered_map>

class ConstraintModel;
class TemporalConstraintViewModel;
class BoxModel;
class DeckModel;
class ProcessSharedModelInterface;

class BoxWidget;
class BoxInspectorSection;
class QFormLayout;
class MetadataWidget;

/*!
 * \brief The ConstraintInspectorWidget class
 *
 * Inherits from InspectorWidgetInterface. Manages an inteface for an Constraint (Timebox) element.
 */
class ConstraintInspectorWidget : public InspectorWidgetBase
{
		Q_OBJECT
	public:
		explicit ConstraintInspectorWidget (TemporalConstraintViewModel* object, QWidget* parent = 0);

		TemporalConstraintViewModel* viewModel() const;
		ConstraintModel* model() const;

	public slots:
		void reloadDisplayedValues()
		{ updateDisplayedValues(m_currentConstraint); }
		void updateDisplayedValues(TemporalConstraintViewModel* obj);

		// These methods ask for creation and the signals originate from other parts of the inspector
		void createProcess(QString processName);
		void createBox();
        void createProcessViewInNewDeck(QString processName);

		void activeBoxChanged(QString box);

        void on_scriptingNameChanged(QString);
        void on_labelChanged(QString);
        void on_commentsChanged(QString);
        void on_colorChanged(QColor);

		// Interface of Constraint
		void on_processCreated(QString processName, id_type<ProcessSharedModelInterface> processId);
		void on_processRemoved(id_type<ProcessSharedModelInterface> processId);

		void on_boxCreated(id_type<BoxModel> boxId);
		void on_boxRemoved(id_type<BoxModel> boxId);

		// These methods are used to display created things
		void displaySharedProcess(ProcessSharedModelInterface*);
		void setupBox(BoxModel*);

	private:
		TemporalConstraintViewModel* m_currentConstraint{};
		QVector<QMetaObject::Connection> m_connections;

        InspectorSectionWidget* m_eventsSection{};
		InspectorSectionWidget* m_durationSection{};

		InspectorSectionWidget* m_processSection{};
		std::vector<InspectorSectionWidget*> m_processesSectionWidgets;

		InspectorSectionWidget* m_boxSection{};
		BoxWidget* m_boxWidget{};
		std::unordered_map<id_type<BoxModel>, BoxInspectorSection*, id_hash<BoxModel>> m_boxesSectionWidgets;

        QVector<QWidget*> m_properties;

        MetadataWidget* m_metadata{};

};
