#include "GroupTableWidget.hpp"

#include "GroupHeaderItem.hpp"
#include "SessionHeaderItem.hpp"
#include "GroupTableCheckbox.hpp"

#include "DistributedScenario/Group.hpp"
#include "session/Session.hpp"

#include "DistributedScenario/Commands/AddClientToGroup.hpp"
#include "DistributedScenario/Commands/RemoveClientFromGroup.hpp"


#include <QLabel>
#include <QTableWidget>
#include <QGridLayout>



GroupTableWidget::GroupTableWidget(const GroupManager* mgr, const Session* session, QWidget* parent):
    QWidget{parent},
    m_mgr{mgr},
    m_session{session},
    m_managerPath{iscore::IDocument::path(m_mgr)},
    m_dispatcher{iscore::IDocument::documentFromObject(m_mgr)->commandStack(), nullptr}
{
    connect(m_mgr, &GroupManager::groupAdded, this, &GroupTableWidget::setup);
    connect(m_mgr, &GroupManager::groupRemoved, this, &GroupTableWidget::setup);
    connect(m_session, &Session::clientsChanged, this, &GroupTableWidget::setup);

    this->setLayout(new QGridLayout);
    this->layout()->addWidget(new QLabel{"Execution table"});

    setup();
}


void GroupTableWidget::setup()
{
    delete m_table;
    m_table = new QTableWidget;
    this->layout()->addWidget(m_table);

    // Groups
    for(unsigned int i = 0; i < m_mgr->groups().size(); i++)
    {
        m_table->insertColumn(i);
        m_table->setHorizontalHeaderItem(i, new GroupHeaderItem{*m_mgr->groups()[i]});
    }

    // Clients
    m_table->insertRow(0); // Local client
    m_table->setVerticalHeaderItem(0, new SessionHeaderItem{m_session->localClient()});

    for(int i = 0; i < m_session->remoteClients().size(); i++)
    {
        m_table->insertRow(i + 1);
        m_table->setVerticalHeaderItem(i + 1, new SessionHeaderItem{*m_session->remoteClients()[i]});
    }

    //Set the data
    using namespace std;
    for(unsigned int i = 0; i < m_mgr->groups().size(); i++)
    {
        for(int j = 0; j < m_session->remoteClients().size() + 1; j++)
        {
            auto cb =  new GroupTableCheckbox;
            m_table->setCellWidget(j, i, cb);
            connect(cb, &GroupTableCheckbox::stateChanged, this, [=] (int state)
            {
                // Lookup id's from the row / column headers
                auto group  = static_cast<GroupHeaderItem*>(m_table->horizontalHeaderItem(i))->group;
                auto client = static_cast<SessionHeaderItem*>(m_table->verticalHeaderItem(j))->client;

                // Find if we have to perform the change.
                auto groupclients = m_mgr->group(group)->clients();
                auto it = std::find(begin(groupclients), end(groupclients), client);

                if(state)
                {
                    if(it != end(groupclients)) return;
                    auto cmd = new AddClientToGroup(ObjectPath{m_managerPath}, client, group);
                    m_dispatcher.submitCommand(cmd);
                }
                else
                {
                    if(it == end(groupclients)) return;
                    auto cmd = new RemoveClientFromGroup(ObjectPath{m_managerPath}, client, group);
                    m_dispatcher.submitCommand(cmd);
                }
            });

        }
    }

    // Handlers

    delete m_groupConnectionContext;
    m_groupConnectionContext = new QObject;

    auto findCheckbox = [this] (int i, id_type<Client> theClient)
    {
        if(theClient == m_session->localClient().id())
        {
            return static_cast<GroupTableCheckbox*>(m_table->cellWidget(0, i));
        }

        for(int j = 0; j < m_session->remoteClients().size(); j++)
        {
            if(static_cast<SessionHeaderItem*>(m_table->verticalHeaderItem(j))->client == theClient)
            {
                return static_cast<GroupTableCheckbox*>(m_table->cellWidget(j+1, i));
            }
        }

        Q_ASSERT(false);
    };

    for(unsigned int i = 0; i < m_mgr->groups().size(); i++)
    {
        connect(m_mgr->groups()[i], &Group::clientAdded,
                m_groupConnectionContext, [=] (id_type<Client> addedClient)
        {
            findCheckbox(i, addedClient)->setState(Qt::Checked);
        });

        connect(m_mgr->groups()[i], &Group::clientRemoved,
                m_groupConnectionContext, [=] (id_type<Client> removedClient)
        {
            findCheckbox(i, removedClient)->setState(Qt::Unchecked);
        });
    }

}