#include <qt/delegationpage.h>
#include <qt/forms/ui_delegationpage.h>
#include <qt/delegationitemmodel.h>
#include <qt/walletmodel.h>
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>
#include <qt/delegationlistwidget.h>
#include <qt/guiutil.h>

#include <QPainter>
#include <QAbstractItemDelegate>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSizePolicy>
#include <QMenu>
#include <QMessageBox>

DelegationPage::DelegationPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DelegationPage),
    m_model(0),
    m_clientModel(0)
{
    ui->setupUi(this);

    m_platformStyle = platformStyle;

    m_removeDelegationPage = new RemoveDelegationPage(this);
    m_addDelegationPage = new AddDelegationPage(this);

    m_removeDelegationPage->setEnabled(false);

    QAction *copyStakerAction = new QAction(tr("Copy super staker address"), this);
    QAction *copyStekerFeeAction = new QAction(tr("Copy super staker fee"), this);
    QAction *copyBlockHeightAction = new QAction(tr("Copy delegation block height"), this);
    QAction *copyDelegateAddressAction = new QAction(tr("Copy delegated address"), this);
    QAction *removeDelegationAction = new QAction(tr("Remove delegation"), this);

    m_delegationList = new DelegationListWidget(platformStyle, this);
    m_delegationList->setContextMenuPolicy(Qt::CustomContextMenu);
    new QVBoxLayout(ui->scrollArea);
    ui->scrollArea->setWidget(m_delegationList);
    ui->scrollArea->setWidgetResizable(true);
    connect(m_delegationList, &DelegationListWidget::removeDelegation, this, &DelegationPage::on_removeDelegation);
    connect(m_delegationList, &DelegationListWidget::addDelegation, this, &DelegationPage::on_addDelegation);

    contextMenu = new QMenu(m_delegationList);
    contextMenu->addAction(copyStakerAction);
    contextMenu->addAction(copyStekerFeeAction);
    contextMenu->addAction(copyBlockHeightAction);
    contextMenu->addAction(copyDelegateAddressAction);
    contextMenu->addAction(removeDelegationAction);

    connect(copyDelegateAddressAction, &QAction::triggered, this, &DelegationPage::copyDelegateAddress);
    connect(copyStekerFeeAction, &QAction::triggered, this, &DelegationPage::copyStekerFee);
    connect(copyBlockHeightAction, &QAction::triggered, this, &DelegationPage::copyBlockHeight);
    connect(copyStakerAction, &QAction::triggered, this, &DelegationPage::copyStakerAddress);
    connect(removeDelegationAction, &QAction::triggered, this, &DelegationPage::removeDelegation);

    connect(m_delegationList, &DelegationListWidget::customContextMenuRequested, this, &DelegationPage::contextualMenu);
}

DelegationPage::~DelegationPage()
{
    delete ui;
}

void DelegationPage::setModel(WalletModel *_model)
{
    m_model = _model;
    m_addDelegationPage->setModel(m_model);
    m_removeDelegationPage->setModel(m_model);
    m_delegationList->setModel(m_model);
    if(m_model && m_model->getDelegationItemModel())
    {
        // Set current delegation
        connect(m_delegationList->delegationModel(), &QAbstractItemModel::dataChanged, this, &DelegationPage::on_dataChanged);
        connect(m_delegationList->delegationModel(), &QAbstractItemModel::rowsInserted, this, &DelegationPage::on_rowsInserted);
        if(m_delegationList->delegationModel()->rowCount() > 0)
        {
            QModelIndex currentDelegation(m_delegationList->delegationModel()->index(0, 0));
            on_currentDelegationChanged(currentDelegation);
        }
    }
}

void DelegationPage::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
    m_addDelegationPage->setClientModel(_clientModel);
    m_removeDelegationPage->setClientModel(_clientModel);
}

void DelegationPage::on_goToRemoveDelegationPage()
{
    m_removeDelegationPage->show();
}

void DelegationPage::on_goToAddDelegationPage()
{
    m_addDelegationPage->show();
}

void DelegationPage::on_currentDelegationChanged(QModelIndex index)
{
    if(m_delegationList->delegationModel())
    {
        if(index.isValid())
        {
            m_selectedDelegationHash = m_delegationList->delegationModel()->data(index, DelegationItemModel::HashRole).toString();
            QString address = m_delegationList->delegationModel()->data(index, DelegationItemModel::AddressRole).toString();
            m_removeDelegationPage->setAddress(address);

            if(!m_removeDelegationPage->isEnabled())
                m_removeDelegationPage->setEnabled(true);
        }
        else
        {
            m_removeDelegationPage->setEnabled(false);
            m_removeDelegationPage->setAddress("");
        }
    }
}

void DelegationPage::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    if(m_delegationList->delegationModel())
    {
        QString delegationHash = m_delegationList->delegationModel()->data(topLeft, DelegationItemModel::HashRole).toString();
        if(m_selectedDelegationHash.isEmpty() ||
                delegationHash == m_selectedDelegationHash)
        {
            on_currentDelegationChanged(topLeft);
        }
    }
}

void DelegationPage::on_currentChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);

    on_currentDelegationChanged(current);
}

void DelegationPage::on_rowsInserted(QModelIndex index, int first, int last)
{
    Q_UNUSED(index);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if(m_delegationList->delegationModel()->rowCount() == 1)
    {
        QModelIndex currentDelegation(m_delegationList->delegationModel()->index(0, 0));
        on_currentDelegationChanged(currentDelegation);
    }
}

void DelegationPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = m_delegationList->indexAt(point);
    if(index.isValid())
    {
        indexMenu = index;
        contextMenu->exec(QCursor::pos());
    }
}

void DelegationPage::copyDelegateAddress()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(DelegationItemModel::AddressRole).toString());
        indexMenu = QModelIndex();
    }
}

void DelegationPage::copyStekerFee()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(DelegationItemModel::FeeRole).toString());
        indexMenu = QModelIndex();
    }
}

void DelegationPage::copyBlockHeight()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(DelegationItemModel::BlockHeightRole).toString());
        indexMenu = QModelIndex();
    }
}

void DelegationPage::copyStakerAddress()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(DelegationItemModel::StakerRole).toString());
        indexMenu = QModelIndex();
    }
}

void DelegationPage::removeDelegation()
{
    QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm delegation removal"), tr("The selected delegation will be removed from the list. Are you sure?"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

    if(btnRetVal == QMessageBox::Yes)
    {
        QModelIndex index = indexMenu;
        std::string sHash = index.data(DelegationItemModel::HashRole).toString().toStdString();
        m_model->wallet().removeDelegationEntry(sHash);
        indexMenu = QModelIndex();
    }
}

void DelegationPage::on_removeDelegation(const QModelIndex &index)
{
    on_currentDelegationChanged(index);
    on_goToRemoveDelegationPage();
}

void DelegationPage::on_addDelegation()
{
    on_goToAddDelegationPage();
}
