#include "RundownCustomCommandWidget.h"

#include "Global.h"

#include "DatabaseManager.h"
#include "DeviceManager.h"
#include "GpiManager.h"
#include "Events/ConnectionStateChangedEvent.h"
#include "Events/Inspector/LabelChangedEvent.h"
#include "Events/Inspector/DeviceChangedEvent.h"

#include <math.h>

#include <QtCore/QObject>
#include <QtCore/QTimer>

RundownCustomCommandWidget::RundownCustomCommandWidget(const LibraryModel& model, QWidget* parent, const QString& color, bool active,
                                                       bool inGroup, bool compactView)
    : QWidget(parent),
      active(active), inGroup(inGroup), compactView(compactView), color(color), model(model), stopControlSubscription(NULL),
      playControlSubscription(NULL), loadControlSubscription(NULL), pauseControlSubscription(NULL), nextControlSubscription(NULL),
      updateControlSubscription(NULL), invokeControlSubscription(NULL), clearControlSubscription(NULL), clearVideolayerControlSubscription(NULL),
      clearChannelControlSubscription(NULL)
{
    setupUi(this);

    this->animation = new ActiveAnimation(this->labelActiveColor);

    this->delayType = DatabaseManager::getInstance().getConfigurationByName("DelayType").getValue();

    setColor(this->color);
    setActive(this->active);
    setCompactView(this->compactView);

    this->labelGroupColor->setVisible(this->inGroup);
    this->labelGroupColor->setStyleSheet(QString("background-color: %1;").arg(Color::DEFAULT_GROUP_COLOR));
    this->labelColor->setStyleSheet(QString("background-color: %1;").arg(Color::DEFAULT_MIXER_COLOR));

    this->labelLabel->setText(this->model.getLabel());
    this->labelDelay->setText(QString("Delay: %1").arg(this->command.getDelay()));
    this->labelDevice->setText(QString("Server: %1").arg(this->model.getDeviceName()));

    this->executeTimer.setSingleShot(true);
    QObject::connect(&this->executeTimer, SIGNAL(timeout()), SLOT(executePlay()));

    QObject::connect(&this->command, SIGNAL(delayChanged(int)), this, SLOT(delayChanged(int)));
    QObject::connect(&this->command, SIGNAL(allowGpiChanged(bool)), this, SLOT(allowGpiChanged(bool)));
    QObject::connect(&this->command, SIGNAL(remoteTriggerIdChanged(const QString&)), this, SLOT(remoteTriggerIdChanged(const QString&)));

    QObject::connect(&DeviceManager::getInstance(), SIGNAL(deviceAdded(CasparDevice&)), this, SLOT(deviceAdded(CasparDevice&)));
    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL)
        QObject::connect(device.data(), SIGNAL(connectionStateChanged(CasparDevice&)), this, SLOT(deviceConnectionStateChanged(CasparDevice&)));

    QObject::connect(GpiManager::getInstance().getGpiDevice().data(), SIGNAL(connectionStateChanged(bool, GpiDevice*)), this, SLOT(gpiConnectionStateChanged(bool, GpiDevice*)));

    checkEmptyDevice();
    checkGpiConnection();
    checkDeviceConnection();

    configureOscSubscriptions();

    qApp->installEventFilter(this);
}

bool RundownCustomCommandWidget::eventFilter(QObject* target, QEvent* event)
{
    if (event->type() == static_cast<QEvent::Type>(Event::EventType::LabelChanged))
    {
        // This event is not for us.
        if (!this->active)
            return false;

        LabelChangedEvent* labelChanged = dynamic_cast<LabelChangedEvent*>(event);
        this->model.setLabel(labelChanged->getLabel());

        this->labelLabel->setText(this->model.getLabel());

        return true;
    }
    else if (event->type() == static_cast<QEvent::Type>(Event::EventType::DeviceChanged))
    {
        // This event is not for us.
        if (!this->active)
            return false;

        // Should we update the device name?
        DeviceChangedEvent* deviceChangedEvent = dynamic_cast<DeviceChangedEvent*>(event);
        if (!deviceChangedEvent->getDeviceName().isEmpty() && deviceChangedEvent->getDeviceName() != this->model.getDeviceName())
        {
            // Disconnect connectionStateChanged() from the old device.
            const QSharedPointer<CasparDevice> oldDevice = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
            if (oldDevice != NULL)
                QObject::disconnect(oldDevice.data(), SIGNAL(connectionStateChanged(CasparDevice&)), this, SLOT(deviceConnectionStateChanged(CasparDevice&)));

            // Update the model with the new device.
            this->model.setDeviceName(deviceChangedEvent->getDeviceName());
            this->labelDevice->setText(QString("Server: %1").arg(this->model.getDeviceName()));

            // Connect connectionStateChanged() to the new device.
            const QSharedPointer<CasparDevice> newDevice = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
            if (newDevice != NULL)
                QObject::connect(newDevice.data(), SIGNAL(connectionStateChanged(CasparDevice&)), this, SLOT(deviceConnectionStateChanged(CasparDevice&)));
        }

        checkEmptyDevice();
        checkDeviceConnection();
    }

    return QObject::eventFilter(target, event);
}

AbstractRundownWidget* RundownCustomCommandWidget::clone()
{
    RundownCustomCommandWidget* widget = new RundownCustomCommandWidget(this->model, this->parentWidget(), this->color, this->active,
                                                                        this->inGroup, this->compactView);

    CustomCommand* command = dynamic_cast<CustomCommand*>(widget->getCommand());
    command->setChannel(this->command.getChannel());
    command->setVideolayer(this->command.getVideolayer());
    command->setDelay(this->command.getDelay());
    command->setAllowGpi(this->command.getAllowGpi());
    command->setAllowRemoteTriggering(this->command.getAllowRemoteTriggering());
    command->setRemoteTriggerId(this->command.getRemoteTriggerId());
    command->setStopCommand(this->command.getStopCommand());
    command->setPlayCommand(this->command.getPlayCommand());
    command->setLoadCommand(this->command.getLoadCommand());
    command->setPauseCommand(this->command.getPauseCommand());
    command->setNextCommand(this->command.getNextCommand());
    command->setUpdateCommand(this->command.getUpdateCommand());
    command->setInvokeCommand(this->command.getInvokeCommand());
    command->setClearCommand(this->command.getClearCommand());
    command->setClearVideolayerCommand(this->command.getClearVideolayerCommand());
    command->setClearChannelCommand(this->command.getClearChannelCommand());
    command->setTriggerOnNext(this->command.getTriggerOnNext());

    return widget;
}

void RundownCustomCommandWidget::setCompactView(bool compactView)
{
    if (compactView)
    {
        this->labelIcon->setFixedSize(Rundown::COMPACT_ICON_WIDTH, Rundown::COMPACT_ICON_HEIGHT);
        this->labelGpiConnected->setFixedSize(Rundown::COMPACT_ICON_WIDTH, Rundown::COMPACT_ICON_HEIGHT);
        this->labelDisconnected->setFixedSize(Rundown::COMPACT_ICON_WIDTH, Rundown::COMPACT_ICON_HEIGHT);
    }
    else
    {
        this->labelIcon->setFixedSize(Rundown::DEFAULT_ICON_WIDTH, Rundown::DEFAULT_ICON_HEIGHT);
        this->labelGpiConnected->setFixedSize(Rundown::DEFAULT_ICON_WIDTH, Rundown::DEFAULT_ICON_HEIGHT);
        this->labelDisconnected->setFixedSize(Rundown::DEFAULT_ICON_WIDTH, Rundown::DEFAULT_ICON_HEIGHT);
    }

    this->compactView = compactView;
}

void RundownCustomCommandWidget::readProperties(boost::property_tree::wptree& pt)
{
    if (pt.count(L"color") > 0) setColor(QString::fromStdWString(pt.get<std::wstring>(L"color")));
}

void RundownCustomCommandWidget::writeProperties(QXmlStreamWriter* writer)
{
    writer->writeTextElement("color", this->color);
}

bool RundownCustomCommandWidget::isGroup() const
{
    return false;
}

bool RundownCustomCommandWidget::isInGroup() const
{
    return this->inGroup;
}

AbstractCommand* RundownCustomCommandWidget::getCommand()
{
    return &this->command;
}

LibraryModel* RundownCustomCommandWidget::getLibraryModel()
{
    return &this->model;
}

void RundownCustomCommandWidget::setActive(bool active)
{
    this->active = active;

    this->animation->stop();

    if (this->active)
        this->labelActiveColor->setStyleSheet(QString("background-color: %1;").arg(Color::DEFAULT_ACTIVE_COLOR));
    else
        this->labelActiveColor->setStyleSheet("");
}

void RundownCustomCommandWidget::setInGroup(bool inGroup)
{
    this->inGroup = inGroup;
    this->labelGroupColor->setVisible(this->inGroup);
}

void RundownCustomCommandWidget::setColor(const QString& color)
{
    this->color = color;
    this->setStyleSheet(QString("#frameItem, #frameStatus { background-color: rgba(%1); }").arg(color));
}

void RundownCustomCommandWidget::checkEmptyDevice()
{
    if (this->labelDevice->text() == "Device: ")
        this->labelDevice->setStyleSheet("color: firebrick;");
    else
        this->labelDevice->setStyleSheet("");
}

bool RundownCustomCommandWidget::executeCommand(Playout::PlayoutType::Type type)
{
    if (type == Playout::PlayoutType::Stop)
        executeStop();
    else if (type == Playout::PlayoutType::Play && !this->command.getTriggerOnNext())
    {
        if (!this->model.getDeviceName().isEmpty()) // The user need to select a device.
        {
            if (this->delayType == Output::DEFAULT_DELAY_IN_FRAMES)
            {
                const QStringList& channelFormats = DatabaseManager::getInstance().getDeviceByName(this->model.getDeviceName()).getChannelFormats().split(",");
                double framesPerSecond = DatabaseManager::getInstance().getFormat(channelFormats[this->command.getChannel() - 1]).getFramesPerSecond().toDouble();

                this->executeTimer.setInterval(floor(this->command.getDelay() * (1000 / framesPerSecond)));
            }
            else if (this->delayType == Output::DEFAULT_DELAY_IN_MILLISECONDS)
            {
                this->executeTimer.setInterval(this->command.getDelay());
            }

            this->executeTimer.start();
        }
    }
    else if (type == Playout::PlayoutType::Load)
        executeLoad();
    else if (type == Playout::PlayoutType::Pause)
        executePause();
    else if (type == Playout::PlayoutType::Next)
    {
        if (this->command.getTriggerOnNext())
            executePlay();
        else
            executeNext();
    }
    else if (type == Playout::PlayoutType::Update)
        executeUpdate();
    else if (type == Playout::PlayoutType::Invoke)
        executeInvoke();
    else if (type == Playout::PlayoutType::Clear)
        executeClear();
    else if (type == Playout::PlayoutType::ClearVideolayer)
        executeClearVideolayer();
    else if (type == Playout::PlayoutType::ClearChannel)
        executeClearChannel();

    if (this->active)
        this->animation->start(1);

    return true;
}

void RundownCustomCommandWidget::executeStop()
{
    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getStopCommand().isEmpty())
        device->sendCommand(this->command.getStopCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getStopCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getStopCommand());
    }
}

void RundownCustomCommandWidget::executePlay()
{
    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getPlayCommand().isEmpty())
        device->sendCommand(this->command.getPlayCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getPlayCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getPlayCommand());
    }
}

void RundownCustomCommandWidget::executeLoad()
{
    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getLoadCommand().isEmpty())
        device->sendCommand(this->command.getLoadCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getLoadCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getLoadCommand());
    }
}

void RundownCustomCommandWidget::executePause()
{
    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getPauseCommand().isEmpty())
        device->sendCommand(this->command.getPauseCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getPauseCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getPauseCommand());
    }
}

void RundownCustomCommandWidget::executeNext()
{
    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getNextCommand().isEmpty())
        device->sendCommand(this->command.getNextCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getNextCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getNextCommand());
    }
}

void RundownCustomCommandWidget::executeUpdate()
{
    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getUpdateCommand().isEmpty())
        device->sendCommand(this->command.getUpdateCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getUpdateCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getUpdateCommand());
    }
}

void RundownCustomCommandWidget::executeInvoke()
{
    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getInvokeCommand().isEmpty())
        device->sendCommand(this->command.getInvokeCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getInvokeCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getInvokeCommand());
    }
}

void RundownCustomCommandWidget::executeClear()
{
    this->executeTimer.stop();

    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getClearCommand().isEmpty())
        device->sendCommand(this->command.getClearCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getClearCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getClearCommand());
    }
}

void RundownCustomCommandWidget::executeClearVideolayer()
{
    this->executeTimer.stop();

    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getClearVideolayerCommand().isEmpty())
        device->sendCommand(this->command.getClearVideolayerCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getClearVideolayerCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getClearVideolayerCommand());
    }
}

void RundownCustomCommandWidget::executeClearChannel()
{
    this->executeTimer.stop();

    this->executeTimer.stop();

    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device != NULL && device->isConnected() && !this->command.getClearChannelCommand().isEmpty())
        device->sendCommand(this->command.getClearChannelCommand());

    foreach (const DeviceModel& model, DeviceManager::getInstance().getDeviceModels())
    {
        if (model.getShadow() == "No")
            continue;

        const QSharedPointer<CasparDevice>  deviceShadow = DeviceManager::getInstance().getDeviceByName(model.getName());
        if (deviceShadow != NULL && deviceShadow->isConnected() && !this->command.getClearChannelCommand().isEmpty())
            deviceShadow->sendCommand(this->command.getClearChannelCommand());
    }
}

void RundownCustomCommandWidget::delayChanged(int delay)
{
    this->labelDelay->setText(QString("Delay: %1").arg(delay));
}

void RundownCustomCommandWidget::checkGpiConnection()
{
    this->labelGpiConnected->setVisible(this->command.getAllowGpi());

    if (GpiManager::getInstance().getGpiDevice()->isConnected())
        this->labelGpiConnected->setPixmap(QPixmap(":/Graphics/Images/GpiConnected.png"));
    else
        this->labelGpiConnected->setPixmap(QPixmap(":/Graphics/Images/GpiDisconnected.png"));
}

void RundownCustomCommandWidget::checkDeviceConnection()
{
    const QSharedPointer<CasparDevice> device = DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName());
    if (device == NULL)
        this->labelDisconnected->setVisible(true);
    else
        this->labelDisconnected->setVisible(!device->isConnected());
}

void RundownCustomCommandWidget::configureOscSubscriptions()
{
    if (DeviceManager::getInstance().getDeviceByName(this->model.getDeviceName()) == NULL)
        return;

    if (this->stopControlSubscription != NULL)
        this->stopControlSubscription->disconnect(); // Disconnect all events.

    if (this->playControlSubscription != NULL)
        this->playControlSubscription->disconnect(); // Disconnect all events.

    if (this->loadControlSubscription != NULL)
        this->loadControlSubscription->disconnect(); // Disconnect all events.

    if (this->nextControlSubscription != NULL)
        this->nextControlSubscription->disconnect(); // Disconnect all events.

    if (this->updateControlSubscription != NULL)
        this->updateControlSubscription->disconnect(); // Disconnect all events.

    if (this->invokeControlSubscription != NULL)
        this->invokeControlSubscription->disconnect(); // Disconnect all events.

    if (this->clearControlSubscription != NULL)
        this->clearControlSubscription->disconnect(); // Disconnect all events.

    if (this->clearVideolayerControlSubscription != NULL)
        this->clearVideolayerControlSubscription->disconnect(); // Disconnect all events.

    if (this->clearChannelControlSubscription != NULL)
        this->clearChannelControlSubscription->disconnect(); // Disconnect all events.

    QString stopControlFilter = Osc::DEFAULT_STOP_CONTROL_FILTER;
    stopControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->stopControlSubscription = new OscSubscription(stopControlFilter, this);
    QObject::connect(this->stopControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(stopControlSubscriptionReceived(const QString&, const QList<QVariant>&)));

    QString playControlFilter = Osc::DEFAULT_PLAY_CONTROL_FILTER;
    playControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->playControlSubscription = new OscSubscription(playControlFilter, this);
    QObject::connect(this->playControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(playControlSubscriptionReceived(const QString&, const QList<QVariant>&)));

    QString loadControlFilter = Osc::DEFAULT_LOAD_CONTROL_FILTER;
    loadControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->loadControlSubscription = new OscSubscription(loadControlFilter, this);
    QObject::connect(this->loadControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(loadControlSubscriptionReceived(const QString&, const QList<QVariant>&)));

    QString pauseControlFilter = Osc::DEFAULT_PAUSE_CONTROL_FILTER;
    pauseControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->pauseControlSubscription = new OscSubscription(pauseControlFilter, this);
    QObject::connect(this->pauseControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(pauseControlSubscriptionReceived(const QString&, const QList<QVariant>&)));

    QString nextControlFilter = Osc::DEFAULT_NEXT_CONTROL_FILTER;
    nextControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->nextControlSubscription = new OscSubscription(nextControlFilter, this);
    QObject::connect(this->nextControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(nextControlSubscriptionReceived(const QString&, const QList<QVariant>&)));

    QString updateControlFilter = Osc::DEFAULT_UPDATE_CONTROL_FILTER;
    updateControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->updateControlSubscription = new OscSubscription(updateControlFilter, this);
    QObject::connect(this->updateControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(updateControlSubscriptionReceived(const QString&, const QList<QVariant>&)));

    QString invokeControlFilter = Osc::DEFAULT_INVOKE_CONTROL_FILTER;
    invokeControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->invokeControlSubscription = new OscSubscription(invokeControlFilter, this);
    QObject::connect(this->pauseControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(invokeControlSubscriptionReceived(const QString&, const QList<QVariant>&)));

    QString clearControlFilter = Osc::DEFAULT_CLEAR_CONTROL_FILTER;
    clearControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->clearControlSubscription = new OscSubscription(clearControlFilter, this);
    QObject::connect(this->clearControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(clearControlSubscriptionReceived(const QString&, const QList<QVariant>&)));

    QString clearVideolayerControlFilter = Osc::DEFAULT_CLEAR_VIDEOLAYER_CONTROL_FILTER;
    clearVideolayerControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->clearVideolayerControlSubscription = new OscSubscription(clearVideolayerControlFilter, this);
    QObject::connect(this->clearVideolayerControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(clearVideolayerControlSubscriptionReceived(const QString&, const QList<QVariant>&)));

    QString clearChannelControlFilter = Osc::DEFAULT_CLEAR_CHANNEL_CONTROL_FILTER;
    clearChannelControlFilter.replace("#UID#", this->command.getRemoteTriggerId());
    this->clearChannelControlSubscription = new OscSubscription(clearChannelControlFilter, this);
    QObject::connect(this->clearChannelControlSubscription, SIGNAL(subscriptionReceived(const QString&, const QList<QVariant>&)),
                     this, SLOT(clearChannelControlSubscriptionReceived(const QString&, const QList<QVariant>&)));
}

void RundownCustomCommandWidget::allowGpiChanged(bool allowGpi)
{
    checkGpiConnection();
}

void RundownCustomCommandWidget::gpiConnectionStateChanged(bool connected, GpiDevice* device)
{
    checkGpiConnection();
}

void RundownCustomCommandWidget::remoteTriggerIdChanged(const QString& remoteTriggerId)
{
    configureOscSubscriptions();

    this->labelRemoteTriggerId->setText(QString("UID: %1").arg(remoteTriggerId));
}

void RundownCustomCommandWidget::deviceConnectionStateChanged(CasparDevice& device)
{
    checkDeviceConnection();
}

void RundownCustomCommandWidget::deviceAdded(CasparDevice& device)
{
    if (DeviceManager::getInstance().getDeviceModelByAddress(device.getAddress()).getName() == this->model.getDeviceName())
        QObject::connect(&device, SIGNAL(connectionStateChanged(CasparDevice&)), this, SLOT(deviceConnectionStateChanged(CasparDevice&)));

    checkDeviceConnection();
}

void RundownCustomCommandWidget::stopControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::Stop);
}

void RundownCustomCommandWidget::playControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::Play);
}

void RundownCustomCommandWidget::loadControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::Load);
}

void RundownCustomCommandWidget::pauseControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::Pause);
}

void RundownCustomCommandWidget::invokeControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::Invoke);
}

void RundownCustomCommandWidget::nextControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::Next);
}

void RundownCustomCommandWidget::updateControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::Update);
}

void RundownCustomCommandWidget::clearControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::Clear);
}

void RundownCustomCommandWidget::clearVideolayerControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::ClearVideolayer);
}

void RundownCustomCommandWidget::clearChannelControlSubscriptionReceived(const QString& predicate, const QList<QVariant>& arguments)
{
    if (this->command.getAllowRemoteTriggering())
        executeCommand(Playout::PlayoutType::ClearChannel);
}
