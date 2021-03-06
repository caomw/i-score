#pragma once
#include <State/Address.hpp>
#include <QWidget>
#include <iscore_plugin_deviceexplorer_export.h>

namespace State
{
class AddressLineEdit;
}

namespace Explorer
{
class DeviceExplorerModel;

/**
 * @brief The AddressEditWidget class
 *
 * Allows editing of an Address.
 * A device explorer model is used for completion.
 *
 */
class ISCORE_PLUGIN_DEVICEEXPLORER_EXPORT AddressEditWidget final : public QWidget
{
        Q_OBJECT
    public:
        AddressEditWidget(DeviceExplorerModel* model, QWidget* parent);

        void setAddress(const State::Address& addr);
        void setAddressString(const QString);

        const State::Address& address() const
        { return m_address; }

        QString addressString() const
        { return m_address.toString(); }


    signals:
        void addressChanged(const State::Address&);

    private:
        void customContextMenuEvent(const QPoint& p);
        void dropEvent(QDropEvent*) override;

        State::AddressLineEdit* m_lineEdit{};
        State::Address m_address;
        DeviceExplorerModel* m_model;
};
}
