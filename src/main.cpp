#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDialog>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QPropertyAnimation>
#include <QProcess>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QVersionNumber>

#include <functional>

#ifdef Q_OS_WIN
static QString findFeatherPhotoUninstallCommand()
{
    const QString registryRoots[] = {
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };

    for (const QString& root : registryRoots) {
        QSettings registry(root, QSettings::NativeFormat);
        const QStringList entries = registry.childGroups();

        for (const QString& entry : entries) {
            registry.beginGroup(entry);
            const QString displayName = registry.value("DisplayName").toString();
            if (displayName.contains("Feather Photo", Qt::CaseInsensitive)) {
                QString command = registry.value("QuietUninstallString").toString();
                if (command.isEmpty())
                    command = registry.value("UninstallString").toString();
                registry.endGroup();
                if (!command.isEmpty())
                    return command;
            }
            registry.endGroup();
        }
    }

    return {};
}

static QString findFeatherPhotoExecutable()
{
    const QString registryRoots[] = {
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };

    QStringList candidateFolders;
    QStringList iconExecutables;

    for (const QString& root : registryRoots) {
        QSettings registry(root, QSettings::NativeFormat);
        const QStringList entries = registry.childGroups();
        for (const QString& entry : entries) {
            registry.beginGroup(entry);
            const QString displayName = registry.value("DisplayName").toString();
            if (displayName.contains("Feather Photo", Qt::CaseInsensitive)) {
                const QString installLocation = registry.value("InstallLocation").toString().trimmed();
                if (!installLocation.isEmpty())
                    candidateFolders.append(QDir::cleanPath(installLocation));

                const QString displayIcon = registry.value("DisplayIcon").toString();
                const QStringList iconParts = QProcess::splitCommand(displayIcon);
                if (!iconParts.isEmpty()) {
                    const QString iconPath = iconParts.first();
                    if (QFileInfo::exists(iconPath))
                        iconExecutables.append(iconPath);
                    candidateFolders.append(QFileInfo(iconPath).absolutePath());
                }

                QString uninstallCommand = registry.value("QuietUninstallString").toString();
                if (uninstallCommand.isEmpty())
                    uninstallCommand = registry.value("UninstallString").toString();
                const QStringList uninstallParts = QProcess::splitCommand(uninstallCommand);
                if (!uninstallParts.isEmpty())
                    candidateFolders.append(QFileInfo(uninstallParts.first()).absolutePath());
            }
            registry.endGroup();
        }
    }

    for (const QString& executable : iconExecutables) {
        if (QFileInfo(executable).isFile())
            return QFileInfo(executable).absoluteFilePath();
    }

    const QStringList knownNames = {
        "FeatherPhoto.exe",
        "Feather Photo.exe",
        "Feather-Photo.exe"
    };
    for (const QString& folder : candidateFolders) {
        const QDir directory(folder);
        for (const QString& name : knownNames) {
            const QString path = directory.filePath(name);
            if (QFileInfo(path).isFile())
                return QFileInfo(path).absoluteFilePath();
        }
    }

    for (const QString& folder : candidateFolders) {
        const QFileInfoList executables = QDir(folder).entryInfoList(
            {"*.exe"}, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& executable : executables) {
            const QString name = executable.fileName();
            if (name.contains("feather", Qt::CaseInsensitive)
                && !name.startsWith("unins", Qt::CaseInsensitive)
                && !name.contains("setup", Qt::CaseInsensitive)) {
                return executable.absoluteFilePath();
            }
        }
    }

    return {};
}

static QString findFeatherPhotoInstalledVersion()
{
    const QString registryRoots[] = {
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };

    for (const QString& root : registryRoots) {
        QSettings registry(root, QSettings::NativeFormat);
        for (const QString& entry : registry.childGroups()) {
            registry.beginGroup(entry);
            const QString displayName = registry.value("DisplayName").toString();
            const QString version = registry.value("DisplayVersion").toString();
            registry.endGroup();
            if (displayName.contains("Feather Photo", Qt::CaseInsensitive))
                return version;
        }
    }

    return {};
}

static bool isNewerVersion(const QString& latest, const QString& installed)
{
    QString latestClean = latest.trimmed();
    QString installedClean = installed.trimmed();
    if (latestClean.startsWith('v', Qt::CaseInsensitive))
        latestClean.remove(0, 1);
    if (installedClean.startsWith('v', Qt::CaseInsensitive))
        installedClean.remove(0, 1);
    latestClean = latestClean.section('-', 0, 0);
    installedClean = installedClean.section('-', 0, 0);

    const QVersionNumber latestNumber = QVersionNumber::fromString(latestClean);
    const QVersionNumber installedNumber = QVersionNumber::fromString(installedClean);
    return !latestNumber.isNull() && !installedNumber.isNull()
        && QVersionNumber::compare(latestNumber, installedNumber) > 0;
}
#endif

class ClickableCard final : public QFrame
{
public:
    using QFrame::QFrame;

    std::function<void()> onClicked;

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && onClicked) {
            onClicked();
            event->accept();
            return;
        }

        QFrame::mousePressEvent(event);
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("Eagle Studio");
    QApplication::setApplicationName("EagleHub");

    QSettings settings;
    QString language = settings.value("language", "pl").toString();

    QMainWindow window;
    window.setWindowTitle("EagleHub");
    window.setWindowIcon(QIcon(":/branding/EagleHubIcon.png"));
    window.resize(1050, 720);

    auto* tabs = new QTabWidget(&window);

    // Zakładka Programy
    auto* programsPage = new QWidget(tabs);
    auto* programsLayout = new QVBoxLayout(programsPage);
    programsLayout->setContentsMargins(28, 24, 28, 24);
    programsLayout->setSpacing(18);

    auto* programsTitle = new QLabel(programsPage);
    programsTitle->setStyleSheet("font-size: 25px; font-weight: 700;");

    auto* programsSubtitle = new QLabel(programsPage);

    auto* refreshButton = new QPushButton(programsPage);
    refreshButton->setObjectName("secondaryButton");
    refreshButton->setFixedWidth(180);

    auto* header = new QHBoxLayout();
    auto* headerText = new QVBoxLayout();
    headerText->addWidget(programsTitle);
    headerText->addWidget(programsSubtitle);

    header->addLayout(headerText);
    header->addStretch();
    header->addWidget(refreshButton);
    programsLayout->addLayout(header);

    auto* cardsGrid = new QGridLayout();
    cardsGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    cardsGrid->setHorizontalSpacing(18);
    cardsGrid->setVerticalSpacing(18);
    programsLayout->addLayout(cardsGrid);
    programsLayout->addStretch();

    auto* downloadBanner = new QFrame(programsPage);
    downloadBanner->setObjectName("downloadBanner");
    downloadBanner->setMinimumHeight(0);
    downloadBanner->setMaximumHeight(0);
    downloadBanner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* bannerLayout = new QVBoxLayout(downloadBanner);
    bannerLayout->setContentsMargins(16, 10, 16, 10);
    bannerLayout->setSpacing(8);

    auto* bannerHeader = new QHBoxLayout();
    auto* downloadBannerLabel = new QLabel(downloadBanner);
    downloadBannerLabel->setObjectName("downloadBannerLabel");
    auto* cancelDownloadButton = new QPushButton(downloadBanner);
    cancelDownloadButton->setObjectName("cancelDownloadButton");
    cancelDownloadButton->setFixedWidth(100);
    bannerHeader->addWidget(downloadBannerLabel, 1);
    bannerHeader->addWidget(cancelDownloadButton);

    auto* downloadProgressBar = new QProgressBar(downloadBanner);
    downloadProgressBar->setObjectName("downloadProgressBar");
    downloadProgressBar->setRange(0, 100);
    downloadProgressBar->setValue(0);
    downloadProgressBar->setTextVisible(false);
    downloadProgressBar->setFixedHeight(8);
    bannerLayout->addLayout(bannerHeader);
    bannerLayout->addWidget(downloadProgressBar);
    programsLayout->addWidget(downloadBanner);

    auto* bannerAnimation = new QPropertyAnimation(downloadBanner, "maximumHeight", &window);
    bannerAnimation->setDuration(280);
    bannerAnimation->setEasingCurve(QEasingCurve::OutCubic);
    bool hideBannerAfterAnimation = false;
    QObject::connect(bannerAnimation, &QPropertyAnimation::finished, downloadBanner, [&]() {
        if (hideBannerAfterAnimation && downloadBanner->maximumHeight() == 0) {
            downloadBanner->hide();
            hideBannerAfterAnimation = false;
        }
    });
    downloadBanner->hide();

    // Kafelek Feather Photo
    auto* photoCard = new ClickableCard(programsPage);
    photoCard->setObjectName("appTile");
    photoCard->setFixedSize(280, 430);
    photoCard->setCursor(Qt::PointingHandCursor);

    auto* tileLayout = new QVBoxLayout(photoCard);
    tileLayout->setContentsMargins(12, 12, 12, 12);
    tileLayout->setSpacing(10);

    auto* photo = new QLabel(photoCard);
    photo->setObjectName("tilePhoto");
    photo->setAlignment(Qt::AlignCenter);
    photo->setFixedSize(256, 190);
    photo->setAttribute(Qt::WA_TransparentForMouseEvents);

    const QPixmap appImage(":/apps/FeatherPhoto.png");
    if (!appImage.isNull()) {
        photo->setPixmap(appImage.scaled(
            180, 180,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    }
    else {
        photo->setText("Feather Photo");
    }

    auto* tileName = new QLabel("Feather Photo", photoCard);
    tileName->setObjectName("tileName");
    tileName->setAlignment(Qt::AlignCenter);
    tileName->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* tileStatus = new QLabel(photoCard);
    tileStatus->setObjectName("tileStatus");
    tileStatus->setAlignment(Qt::AlignCenter);
    tileStatus->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* downloadButton = new QPushButton(photoCard);
    downloadButton->setObjectName("primaryButton");
    downloadButton->setFixedHeight(40);

    auto* uninstallButton = new QPushButton(photoCard);
    uninstallButton->setObjectName("secondaryButton");
    uninstallButton->setFixedHeight(40);
    uninstallButton->hide();

    auto* launchButton = new QPushButton(photoCard);
    launchButton->setObjectName("primaryButton");
    launchButton->setFixedHeight(40);
    launchButton->hide();

    auto* stopButton = new QPushButton(photoCard);
    stopButton->setObjectName("dangerButton");
    stopButton->setFixedHeight(40);
    stopButton->hide();

    tileLayout->addWidget(photo);
    tileLayout->addWidget(tileName);
    tileLayout->addWidget(tileStatus);
    tileLayout->addStretch();
    tileLayout->addWidget(launchButton);
    tileLayout->addWidget(stopButton);
    tileLayout->addWidget(downloadButton);
    tileLayout->addWidget(uninstallButton);

    cardsGrid->addWidget(photoCard, 0, 0);

    QNetworkAccessManager network(&window);
    QPointer<QNetworkReply> activeReply;
    QPointer<QProcess> appProcess;
    QPointer<QPushButton> detailsInstallButton;
    QPointer<QPushButton> detailsUninstallButton;
    QPointer<QPushButton> detailsLaunchButton;
    QPointer<QPushButton> detailsStopButton;
    QPointer<QLabel> detailsInstallStatus;
    QPointer<QLabel> detailsVersionLabel;
    QFile* outputFile = nullptr;
    QString downloadedInstallerPath;
    QElapsedTimer downloadClock;
    bool installed = false;
    QString installedVersion;
    QString latestVersion;
    bool updateAvailable = false;
    QPointer<QNetworkReply> updateCheckReply;
    std::function<void()> startInstall;
    std::function<void()> checkForUpdates;
    std::function<void()> uninstallApp;
    std::function<void()> launchApp;
    std::function<void()> stopApp;

#ifdef Q_OS_WIN
    installed = !findFeatherPhotoUninstallCommand().isEmpty();
    installedVersion = findFeatherPhotoInstalledVersion();
#endif

    // Kliknięcie kafelka otwiera szczegóły aplikacji.
    photoCard->onClicked = [&window, &language, &installed, &startInstall,
                            &uninstallApp, &launchApp, &stopApp, &appProcess,
                            &detailsInstallButton, &detailsUninstallButton,
                            &detailsLaunchButton,
                            &detailsStopButton,
                            &detailsInstallStatus, &detailsVersionLabel,
                            &latestVersion, &installedVersion, &updateAvailable]() {
        const bool polish = language == "pl";

        QDialog details(&window);
        details.setWindowTitle("Feather Photo");
        details.resize(850, 650);

        auto* layout = new QVBoxLayout(&details);
        layout->setContentsMargins(24, 22, 24, 22);
        layout->setSpacing(14);

        auto* title = new QLabel("Feather Photo", &details);
        title->setStyleSheet("font-size: 26px; font-weight: 700;");

        auto* description = new QLabel(
            polish
            ? "Edytor zdjęć z narzędziami do rysowania, zaznaczania "
            "i pracy na warstwach."
            : "A photo editor with drawing tools, selection and layers.",
            &details
        );
        description->setWordWrap(true);

        auto* screenshotsTitle = new QLabel(
            polish ? "Zrzuty ekranu" : "Screenshots",
            &details
        );
        screenshotsTitle->setStyleSheet("font-size: 18px; font-weight: 600;");

        auto* gallery = new QHBoxLayout();
        gallery->setSpacing(12);

        const QStringList screenshotResources = {
            ":/screenshots/FeatherPhoto-1.png",
            ":/screenshots/FeatherPhoto-2.png",
            ":/screenshots/FeatherPhoto-3.png"
        };

        for (int i = 0; i < screenshotResources.size(); ++i) {
            auto* preview = new QLabel(&details);
            preview->setAlignment(Qt::AlignCenter);
            preview->setFixedSize(240, 150);
            preview->setStyleSheet(
                "background: #252f3c;"
                "border: 1px solid #344153;"
                "border-radius: 8px;"
                "color: #aab7c8;"
            );

            const QPixmap screenshot(screenshotResources.at(i));
            if (!screenshot.isNull()) {
                preview->setPixmap(screenshot.scaled(
                    preview->size() - QSize(12, 12),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                ));
            }
            else {
                preview->setText(
                    polish
                    ? QString("Nie udało się wczytać zrzutu %1").arg(i + 1)
                    : QString("Could not load screenshot %1").arg(i + 1)
                );
                preview->setWordWrap(true);
            }

            gallery->addWidget(preview);
        }

        auto* version = new QLabel(
            polish
                ? QString("Zainstalowana wersja: %1 · najnowsza: %2")
                      .arg(installedVersion.isEmpty() ? "—" : installedVersion,
                           latestVersion.isEmpty() ? "sprawdzanie…" : latestVersion)
                : QString("Installed: %1 · latest: %2")
                      .arg(installedVersion.isEmpty() ? "—" : installedVersion,
                           latestVersion.isEmpty() ? "checking…" : latestVersion),
            &details
        );
        detailsVersionLabel = version;

        auto* installStatus = new QLabel(
            installed
            ? (polish ? "Feather Photo jest zainstalowany." : "Feather Photo is installed.")
            : (polish ? "Feather Photo nie jest zainstalowany." : "Feather Photo is not installed."),
            &details
        );
        detailsInstallStatus = installStatus;

        auto* buttons = new QHBoxLayout();
        auto* closeButton = new QPushButton(
            polish ? "Zamknij" : "Close",
            &details
        );
        auto* installButton = new QPushButton(
            !installed
                ? (polish ? "Pobierz i zainstaluj" : "Download and install")
                : (updateAvailable ? (polish ? "Aktualizuj" : "Update") : ""),
            &details
        );
        installButton->setObjectName("primaryButton");
        installButton->setVisible(!installed || updateAvailable);
        detailsInstallButton = installButton;

        auto* uninstallProgramButton = new QPushButton(
            polish ? "Odinstaluj" : "Uninstall",
            &details
        );
        uninstallProgramButton->setObjectName("secondaryButton");
        uninstallProgramButton->setVisible(installed);
        detailsUninstallButton = uninstallProgramButton;

        auto* runButton = new QPushButton(
            polish ? "Uruchom" : "Run",
            &details
        );
        runButton->setObjectName("primaryButton");
        runButton->setVisible(installed && (!appProcess || appProcess->state() != QProcess::Running));
        detailsLaunchButton = runButton;

        auto* stopProgramButton = new QPushButton(
            polish ? "Zatrzymaj" : "Stop",
            &details
        );
        stopProgramButton->setObjectName("dangerButton");
        stopProgramButton->setVisible(appProcess && appProcess->state() == QProcess::Running);
        detailsStopButton = stopProgramButton;

        buttons->addStretch();
        buttons->addWidget(runButton);
        buttons->addWidget(stopProgramButton);
        buttons->addWidget(closeButton);
        buttons->addWidget(installButton);
        buttons->addWidget(uninstallProgramButton);

        layout->addWidget(title);
        layout->addWidget(description);
        layout->addWidget(screenshotsTitle);
        layout->addLayout(gallery);
        layout->addWidget(version);
        layout->addWidget(installStatus);
        layout->addStretch();
        layout->addLayout(buttons);

        QObject::connect(
            closeButton,
            &QPushButton::clicked,
            &details,
            &QDialog::accept
        );

        QObject::connect(runButton, &QPushButton::clicked, &details,
                         [&details, &launchApp]() {
            details.accept();
            if (launchApp)
                launchApp();
        });

        QObject::connect(stopProgramButton, &QPushButton::clicked, &details,
                         [&details, &stopApp]() {
            details.accept();
            if (stopApp)
                stopApp();
        });

        QObject::connect(installButton, &QPushButton::clicked, &details,
                         [&details, &startInstall]() {
            details.accept();
            if (startInstall)
                startInstall();
        });

        QObject::connect(uninstallProgramButton, &QPushButton::clicked, &details,
                         [&details, &uninstallApp]() {
            details.accept();
            if (uninstallApp)
                uninstallApp();
        });

        QObject::connect(&details, &QDialog::finished, &details,
                         [&detailsInstallButton, &detailsUninstallButton, &detailsLaunchButton,
                          &detailsStopButton, &detailsInstallStatus, &detailsVersionLabel]() {
            detailsInstallButton = nullptr;
            detailsUninstallButton = nullptr;
            detailsLaunchButton = nullptr;
            detailsStopButton = nullptr;
            detailsInstallStatus = nullptr;
            detailsVersionLabel = nullptr;
        });

        details.exec();
        };

    // Zakładka Ustawienia
    auto* settingsPage = new QWidget(tabs);
    auto* settingsLayout = new QVBoxLayout(settingsPage);
    settingsLayout->setContentsMargins(28, 24, 28, 24);
    settingsLayout->setSpacing(14);

    auto* settingsTitle = new QLabel(settingsPage);
    settingsTitle->setStyleSheet("font-size: 25px; font-weight: 700;");

    auto* themeLabel = new QLabel(settingsPage);
    auto* themeBox = new QComboBox(settingsPage);
    themeBox->addItem("Ciemny", "dark");
    themeBox->addItem("Jasny", "light");
    themeBox->addItem("Systemowy", "system");

    auto* languageLabel = new QLabel(settingsPage);
    auto* languageBox = new QComboBox(settingsPage);
    languageBox->addItem("Polski", "pl");
    languageBox->addItem("English", "en");

    auto* folderLabel = new QLabel(settingsPage);
    auto* folderEdit = new QLineEdit(settingsPage);
    auto* chooseFolderButton = new QPushButton(settingsPage);

    auto* folderRow = new QHBoxLayout();
    folderRow->addWidget(folderEdit, 1);
    folderRow->addWidget(chooseFolderButton);

    settingsLayout->addWidget(settingsTitle);
    settingsLayout->addWidget(themeLabel);
    settingsLayout->addWidget(themeBox);
    settingsLayout->addWidget(languageLabel);
    settingsLayout->addWidget(languageBox);
    settingsLayout->addWidget(folderLabel);
    settingsLayout->addLayout(folderRow);
    settingsLayout->addStretch();

    tabs->addTab(programsPage, "");
    tabs->addTab(settingsPage, "");

    auto* central = new QWidget(&window);
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(12, 12, 12, 12);
    centralLayout->addWidget(tabs);
    window.setCentralWidget(central);

    const QString defaultFolder =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
        + "/EagleHub";

    folderEdit->setText(
        settings.value("downloadFolder", defaultFolder).toString()
    );
    themeBox->setCurrentIndex(
        themeBox->findData(settings.value("theme", "dark").toString())
    );
    languageBox->setCurrentIndex(
        languageBox->findData(language)
    );

    auto formatEta = [](qint64 seconds) {
        const qint64 hours = seconds / 3600;
        const qint64 minutes = (seconds % 3600) / 60;
        const qint64 remainingSeconds = seconds % 60;
        return QStringLiteral("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(remainingSeconds, 2, 10, QLatin1Char('0'));
    };

    auto updateInstallUi = [&]() {
        const bool polish = language == "pl";
        const bool running = appProcess && appProcess->state() == QProcess::Running;
        const bool hasInstallAction = !installed || updateAvailable;
        launchButton->setText(polish ? "Uruchom" : "Run");
        launchButton->setVisible(installed && !running);
        stopButton->setText(polish ? "Zatrzymaj" : "Stop");
        stopButton->setVisible(running);
        downloadButton->setVisible(hasInstallAction);
        downloadButton->setText(installed
            ? (polish ? "Aktualizuj" : "Update")
            : (polish ? "Pobierz" : "Download"));
        uninstallButton->setVisible(installed);
        uninstallButton->setText(polish ? "Odinstaluj" : "Uninstall");
        tileStatus->setText(updateAvailable
            ? (polish ? QString("Dostępna aktualizacja %1").arg(latestVersion)
                      : QString("Update available: %1").arg(latestVersion))
            : (installed
                ? (polish ? QString("Zainstalowany · %1").arg(installedVersion)
                          : QString("Installed · %1").arg(installedVersion))
                : (polish ? "Gotowy do pobrania" : "Ready to download")));
        if (detailsInstallButton) {
            detailsInstallButton->setVisible(hasInstallAction);
            detailsInstallButton->setText(installed
                ? (polish ? "Aktualizuj" : "Update")
                : (polish ? "Pobierz i zainstaluj" : "Download and install"));
        }
        if (detailsUninstallButton)
            detailsUninstallButton->setVisible(installed);
        if (detailsLaunchButton) {
            detailsLaunchButton->setText(polish ? "Uruchom" : "Run");
            detailsLaunchButton->setVisible(installed && !running);
        }
        if (detailsStopButton) {
            detailsStopButton->setText(polish ? "Zatrzymaj" : "Stop");
            detailsStopButton->setVisible(running);
        }
        if (detailsInstallStatus)
            detailsInstallStatus->setText(installed
                ? (polish ? "Feather Photo jest zainstalowany." : "Feather Photo is installed.")
                : (polish ? "Feather Photo nie jest zainstalowany." : "Feather Photo is not installed."));
        if (detailsVersionLabel) {
            const QString current = installedVersion.isEmpty() ? "—" : installedVersion;
            const QString latest = latestVersion.isEmpty()
                ? (polish ? "sprawdzanie…" : "checking…")
                : latestVersion;
            detailsVersionLabel->setText(polish
                ? QString("Zainstalowana wersja: %1 · najnowsza: %2").arg(current, latest)
                : QString("Installed: %1 · latest: %2").arg(current, latest));
        }
    };

    auto showDownloadBanner = [&]() {
        if (downloadBanner->isVisible()) {
            const bool alreadyShowing = bannerAnimation->state() == QAbstractAnimation::Running
                && bannerAnimation->endValue().toInt() == 86;
            const bool alreadyOpen = bannerAnimation->state() != QAbstractAnimation::Running
                && downloadBanner->maximumHeight() >= 86;
            if (alreadyShowing || alreadyOpen)
                return;
        }
        hideBannerAfterAnimation = false;
        cancelDownloadButton->setEnabled(true);
        bannerAnimation->stop();
        downloadBanner->show();
        bannerAnimation->setStartValue(downloadBanner->maximumHeight());
        bannerAnimation->setEndValue(86);
        bannerAnimation->start();
    };

    auto hideDownloadBanner = [&]() {
        if (!downloadBanner->isVisible())
            return;
        const bool alreadyClosing = bannerAnimation->state() == QAbstractAnimation::Running
            && bannerAnimation->endValue().toInt() == 0;
        if (alreadyClosing)
            return;
        cancelDownloadButton->setEnabled(false);
        bannerAnimation->stop();
        hideBannerAfterAnimation = true;
        bannerAnimation->setStartValue(downloadBanner->maximumHeight());
        bannerAnimation->setEndValue(0);
        bannerAnimation->start();
    };

    auto setTransferStatus = [&](const QString& message, bool showProgress) {
        tileStatus->setText(showProgress
            ? (language == "pl" ? "Pobieranie…" : "Downloading…")
            : message);
        downloadBannerLabel->setText(message);
        if (showProgress)
            showDownloadBanner();
    };

    QTimer installStateTimer;
    installStateTimer.setInterval(1500);
    int installStatePollCount = 0;
    QObject::connect(&installStateTimer, &QTimer::timeout, &window, [&]() {
#ifdef Q_OS_WIN
        const bool nowInstalled = !findFeatherPhotoUninstallCommand().isEmpty();
        const QString nowVersion = nowInstalled ? findFeatherPhotoInstalledVersion() : QString{};
        const bool stateChanged = nowInstalled != installed || nowVersion != installedVersion;
        if (stateChanged) {
            installed = nowInstalled;
            installedVersion = nowVersion;
            updateAvailable = installed && !latestVersion.isEmpty()
                && isNewerVersion(latestVersion, installedVersion);
            updateInstallUi();
            if (!installed || !updateAvailable)
                installStateTimer.stop();
        }
        if (++installStatePollCount >= 240)
            installStateTimer.stop();
#else
        installStateTimer.stop();
#endif
    });

    QObject::connect(cancelDownloadButton, &QPushButton::clicked, &window, [&]() {
        if (activeReply)
            activeReply->abort();
    });

    auto showTransferError = [&](const QString& message) {
        tileStatus->setText(language == "pl" ? "Pobieranie nie powiodło się" : "Download failed");
        hideDownloadBanner();
        QMessageBox::warning(&window, "EagleHub", message);
    };

    auto beginAssetDownload = [&](const QUrl& url, const QString& assetName) {
        const QString targetFolder = folderEdit->text().trimmed();
        if (targetFolder.isEmpty() || !QDir().mkpath(targetFolder)) {
            showTransferError(language == "pl"
                ? "Nie można utworzyć wybranego folderu pobierania."
                : "Could not create the selected download folder.");
            return;
        }

        const QString safeName = QFileInfo(assetName).fileName();
        downloadedInstallerPath = QDir(targetFolder).filePath(safeName);
        QFile::remove(downloadedInstallerPath);
        outputFile = new QFile(downloadedInstallerPath, &window);
        if (!outputFile->open(QIODevice::WriteOnly)) {
            outputFile->deleteLater();
            outputFile = nullptr;
            showTransferError(language == "pl"
                ? "Nie można zapisać instalatora w wybranym folderze."
                : "Could not save the installer in the selected folder.");
            return;
        }

        downloadClock.start();
        downloadProgressBar->setRange(0, 0);
        setTransferStatus(language == "pl" ? "Pobieranie instalatora…" : "Downloading installer…", true);

        QNetworkRequest request(url);
        request.setRawHeader("User-Agent", "EagleHub/0.1");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply* reply = network.get(request);
        activeReply = reply;

        QObject::connect(reply, &QIODevice::readyRead, reply, [&, reply]() {
            if (!outputFile || outputFile->write(reply->readAll()) < 0)
                reply->abort();
        });

        QObject::connect(reply, &QNetworkReply::downloadProgress, reply,
                         [&, reply](qint64 received, qint64 total) {
            if (total <= 0) {
                downloadProgressBar->setRange(0, 0);
                setTransferStatus(language == "pl"
                    ? QString("Pobrano %1 MB · pozostały czas nieznany")
                          .arg(received / (1024.0 * 1024.0), 0, 'f', 1)
                    : QString("Downloaded %1 MB · remaining time unknown")
                          .arg(received / (1024.0 * 1024.0), 0, 'f', 1), true);
                return;
            }

            const int percent = static_cast<int>((received * 100) / total);
            downloadProgressBar->setRange(0, 100);
            downloadProgressBar->setValue(percent);

            const qint64 elapsedMs = qMax<qint64>(1, downloadClock.elapsed());
            const double bytesPerSecond = received * 1000.0 / elapsedMs;
            QString remaining = language == "pl" ? "obliczanie…" : "calculating…";
            if (bytesPerSecond > 0.0) {
                const qint64 seconds = static_cast<qint64>((total - received) / bytesPerSecond);
                remaining = formatEta(seconds);
            }

            const QString message = language == "pl"
                ? QString("Pobieranie: %1% · pozostało %2").arg(percent).arg(remaining)
                : QString("Downloading: %1% · %2 remaining").arg(percent).arg(remaining);
            setTransferStatus(message, true);
        });

        QObject::connect(reply, &QNetworkReply::finished, &window, [&, reply]() {
            if (outputFile) {
                const QByteArray lastData = reply->readAll();
                if (!lastData.isEmpty())
                    outputFile->write(lastData);
                outputFile->flush();
                outputFile->close();
            }

            const QNetworkReply::NetworkError error = reply->error();
            const QString errorText = reply->errorString();
            activeReply = nullptr;
            reply->deleteLater();

            if (outputFile) {
                outputFile->deleteLater();
                outputFile = nullptr;
            }

            if (error != QNetworkReply::NoError) {
                QFile::remove(downloadedInstallerPath);
                hideDownloadBanner();
                if (error != QNetworkReply::OperationCanceledError) {
                    tileStatus->setText(language == "pl" ? "Pobieranie nie powiodło się" : "Download failed");
                    QMessageBox::warning(&window, "EagleHub", language == "pl"
                        ? QString("Nie udało się pobrać instalatora: %1").arg(errorText)
                        : QString("Could not download the installer: %1").arg(errorText));
                }
                else {
                    tileStatus->setText(language == "pl" ? "Pobieranie anulowane" : "Download cancelled");
                }
                return;
            }

            downloadProgressBar->setRange(0, 100);
            downloadProgressBar->setValue(100);
            cancelDownloadButton->setEnabled(false);
#ifdef Q_OS_WIN
            const bool launched = QProcess::startDetached(downloadedInstallerPath, QStringList{});
            if (!launched) {
                tileStatus->setText(language == "pl" ? "Pobrano instalator" : "Installer downloaded");
                QMessageBox::warning(&window, "EagleHub", language == "pl"
                    ? QString("Pobieranie zakończone, ale nie udało się uruchomić instalatora. Plik: %1")
                          .arg(downloadedInstallerPath)
                    : QString("Download finished, but the installer could not be started. File: %1")
                          .arg(downloadedInstallerPath));
                return;
            }
            tileStatus->setText(language == "pl"
                ? "Instalator uruchomiony — dokończ instalację"
                : "Installer launched — finish the installation");
            downloadBannerLabel->setText(tileStatus->text());
            QTimer::singleShot(4500, &window, hideDownloadBanner);
            installStatePollCount = 0;
            installStateTimer.start();
#else
            QMessageBox::information(&window, "EagleHub", language == "pl"
                ? QString("Instalator pobrany: %1").arg(downloadedInstallerPath)
                : QString("Installer downloaded: %1").arg(downloadedInstallerPath));
#endif
        });
    };

    startInstall = [&]() {
        if (activeReply)
            return;

#ifndef Q_OS_WIN
        QMessageBox::information(&window, "EagleHub", "Installer launch is available on Windows.");
        return;
#else
        downloadProgressBar->setRange(0, 0);
        cancelDownloadButton->setEnabled(true);
        setTransferStatus(language == "pl"
            ? "Sprawdzanie najnowszej wersji…"
            : "Checking the latest version…", true);

        QNetworkRequest request(QUrl("https://api.github.com/repos/MasterMarcel272/Feather-Photo/releases/latest"));
        request.setRawHeader("User-Agent", "EagleHub/0.1");
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply* reply = network.get(request);
        activeReply = reply;

        QObject::connect(reply, &QNetworkReply::finished, &window, [&, reply]() {
            const QByteArray body = reply->readAll();
            const QNetworkReply::NetworkError error = reply->error();
            const QString errorText = reply->errorString();
            activeReply = nullptr;
            reply->deleteLater();

            if (error != QNetworkReply::NoError) {
                if (error == QNetworkReply::OperationCanceledError) {
                    tileStatus->setText(language == "pl" ? "Pobieranie anulowane" : "Download cancelled");
                    hideDownloadBanner();
                    return;
                }
                tileStatus->setText(language == "pl" ? "Nie można połączyć z GitHubem" : "Could not reach GitHub");
                hideDownloadBanner();
                QMessageBox::warning(&window, "EagleHub", language == "pl"
                    ? QString("Nie udało się sprawdzić wydania Feather Photo: %1").arg(errorText)
                    : QString("Could not check the Feather Photo release: %1").arg(errorText));
                return;
            }

            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                showTransferError(language == "pl"
                    ? "GitHub zwrócił nieprawidłowe dane o wydaniu."
                    : "GitHub returned invalid release data.");
                return;
            }

            const QJsonArray assets = document.object().value("assets").toArray();
            QJsonObject selectedAsset;
            for (const QJsonValue& value : assets) {
                const QJsonObject asset = value.toObject();
                const QString name = asset.value("name").toString();
                if (!name.endsWith(".exe", Qt::CaseInsensitive))
                    continue;
                if (selectedAsset.isEmpty()
                    || name.contains("setup", Qt::CaseInsensitive)
                    || name.contains("installer", Qt::CaseInsensitive)) {
                    selectedAsset = asset;
                    if (name.contains("setup", Qt::CaseInsensitive)
                        || name.contains("installer", Qt::CaseInsensitive))
                        break;
                }
            }

            const QString assetName = selectedAsset.value("name").toString();
            const QUrl assetUrl(selectedAsset.value("browser_download_url").toString());
            if (selectedAsset.isEmpty() || !assetUrl.isValid() || assetUrl.isEmpty()) {
                showTransferError(language == "pl"
                    ? "W najnowszym wydaniu GitHub nie ma instalatora .exe."
                    : "The latest GitHub release does not contain a .exe installer.");
                return;
            }

            beginAssetDownload(assetUrl, assetName);
        });
#endif
    };

    uninstallApp = [&]() {
#ifdef Q_OS_WIN
        const QString command = findFeatherPhotoUninstallCommand();
        if (command.isEmpty()) {
            installed = false;
            updateInstallUi();
            QMessageBox::information(&window, "EagleHub", language == "pl"
                ? "Nie znaleziono instalatora odinstalowującego Feather Photo."
                : "Could not find the Feather Photo uninstaller.");
            return;
        }

        const auto answer = QMessageBox::question(&window, "EagleHub",
            language == "pl" ? "Czy na pewno odinstalować Feather Photo?"
                              : "Are you sure you want to uninstall Feather Photo?");
        if (answer != QMessageBox::Yes)
            return;

        QStringList parts = QProcess::splitCommand(command);
        if (parts.isEmpty()) {
            QMessageBox::warning(&window, "EagleHub", language == "pl"
                ? "Nieprawidłowa komenda deinstalatora."
                : "The uninstaller command is invalid.");
            return;
        }

        const QString program = parts.takeFirst();
        if (QFileInfo(program).fileName().contains("msiexec", Qt::CaseInsensitive)) {
            for (QString& argument : parts) {
                if (argument.startsWith("/I", Qt::CaseInsensitive))
                    argument.replace(0, 2, "/X");
            }
        }

        if (!QProcess::startDetached(program, parts)) {
            QMessageBox::warning(&window, "EagleHub", language == "pl"
                ? "Nie udało się uruchomić deinstalatora."
                : "Could not start the uninstaller.");
            return;
        }

        tileStatus->setText(language == "pl" ? "Trwa odinstalowywanie…" : "Uninstalling…");
        installStatePollCount = 0;
        installStateTimer.start();
#endif
    };

    launchApp = [&]() {
#ifdef Q_OS_WIN
        if (appProcess && appProcess->state() == QProcess::Running) {
            tileStatus->setText(language == "pl" ? "Feather Photo już działa" : "Feather Photo is already running");
            return;
        }

        const QString executable = findFeatherPhotoExecutable();
        if (executable.isEmpty()) {
            QMessageBox::warning(&window, "EagleHub", language == "pl"
                ? "Nie udało się znaleźć pliku programu Feather Photo."
                : "Could not find the Feather Photo executable.");
            return;
        }

        const QString workingDirectory = QFileInfo(executable).absolutePath();
        auto* process = new QProcess(&window);
        process->setProgram(executable);
        process->setWorkingDirectory(workingDirectory);
        appProcess = process;

        QObject::connect(process, &QProcess::started, &window, [&]() {
            updateInstallUi();
            tileStatus->setText(language == "pl" ? "Feather Photo uruchomiony" : "Feather Photo launched");
        });
        QObject::connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                         &window, [&, process](int, QProcess::ExitStatus) {
            if (appProcess == process)
                appProcess = nullptr;
            process->deleteLater();
            updateInstallUi();
        });
        QObject::connect(process, &QProcess::errorOccurred, &window,
                         [&, process](QProcess::ProcessError error) {
            if (error != QProcess::FailedToStart)
                return;
            if (appProcess == process)
                appProcess = nullptr;
            process->deleteLater();
            updateInstallUi();
            QMessageBox::warning(&window, "EagleHub", language == "pl"
                ? "Nie udało się uruchomić Feather Photo."
                : "Could not launch Feather Photo.");
        });
        process->start();
#endif
    };

    stopApp = [&]() {
#ifdef Q_OS_WIN
        if (!appProcess || appProcess->state() != QProcess::Running) {
            updateInstallUi();
            return;
        }

        QPointer<QProcess> process = appProcess;
        tileStatus->setText(language == "pl" ? "Zamykanie Feather Photo…" : "Closing Feather Photo…");
        process->terminate();
        QTimer::singleShot(4000, &window, [process]() {
            if (process && process->state() != QProcess::NotRunning)
                process->kill();
        });
#endif
    };

    checkForUpdates = [&]() {
#ifdef Q_OS_WIN
        if (updateCheckReply)
            return;

        if (installed && installedVersion.isEmpty())
            installedVersion = findFeatherPhotoInstalledVersion();
        if (installed && !activeReply)
            tileStatus->setText(language == "pl" ? "Sprawdzanie aktualizacji…" : "Checking for updates…");

        QNetworkRequest request(QUrl("https://api.github.com/repos/MasterMarcel272/Feather-Photo/releases/latest"));
        request.setRawHeader("User-Agent", "EagleHub/0.1");
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply* reply = network.get(request);
        updateCheckReply = reply;

        QObject::connect(reply, &QNetworkReply::finished, &window, [&, reply]() {
            const QByteArray body = reply->readAll();
            const QNetworkReply::NetworkError error = reply->error();
            updateCheckReply = nullptr;
            reply->deleteLater();
            if (error != QNetworkReply::NoError)
                return;

            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject())
                return;

            const QJsonObject release = document.object();
            latestVersion = release.value("tag_name").toString();
            if (latestVersion.isEmpty())
                latestVersion = release.value("name").toString();

            if (installedVersion.isEmpty())
                installedVersion = findFeatherPhotoInstalledVersion();
            installed = !findFeatherPhotoUninstallCommand().isEmpty();
            updateAvailable = installed && isNewerVersion(latestVersion, installedVersion);
            updateInstallUi();
        });
#endif
    };

    QObject::connect(&app, &QApplication::aboutToQuit, &app, [&]() {
        // Pozwól Feather Photo działać po zamknięciu launchera.
        if (appProcess)
            appProcess->setParent(nullptr);
    });

    updateInstallUi();

    auto applyTheme = [&]() {
        const QString theme = themeBox->currentData().toString();

        if (theme == "system") {
            app.setStyleSheet("");
            return;
        }

        if (theme == "light") {
            app.setStyleSheet(R"(
                QWidget {
                    background-color: #f3f5f8;
                    color: #202733;
                    font-family: "Segoe UI";
                }
                QLabel { background: transparent; }
                QTabWidget::pane {
                    border: 1px solid #d2d8e1;
                    border-radius: 8px;
                }
                QTabBar::tab {
                    background: #e3e8ef;
                    padding: 10px 18px;
                }
                QTabBar::tab:selected {
                    background: #3478f6;
                    color: white;
                }
                QFrame#appTile {
                    background: white;
                    border: 1px solid #d2d8e1;
                    border-radius: 12px;
                }
                QFrame#appTile:hover {
                    border: 2px solid #3478f6;
                }
                QLabel#tilePhoto {
                    background: #e3e8ef;
                    border-radius: 8px;
                }
                QLabel#tileName {
                    font-size: 16px;
                    font-weight: 600;
                }
                QFrame#downloadBanner {
                    background: #ffffff;
                    border: 1px solid #d2d8e1;
                    border-radius: 10px;
                }
                QLabel#downloadBannerLabel {
                    background: transparent;
                    font-weight: 600;
                }
                QProgressBar#downloadProgressBar {
                    background: #e3e8ef;
                    border: 0;
                    border-radius: 4px;
                }
                QProgressBar#downloadProgressBar::chunk {
                    background: #3478f6;
                    border-radius: 4px;
                }
                QPushButton#cancelDownloadButton {
                    background: #e3e8ef;
                    color: #202733;
                }
                QLineEdit, QComboBox {
                    background: white;
                    border: 1px solid #c7cfda;
                    border-radius: 6px;
                    padding: 8px;
                }
                QPushButton {
                    background: #e3e8ef;
                    border: 0;
                    border-radius: 6px;
                    padding: 9px 14px;
                }
                QPushButton#primaryButton {
                    background: #3478f6;
                    color: white;
                    font-weight: 600;
                }
                QPushButton#secondaryButton {
                    background: #e3e8ef;
                    color: #202733;
                    font-weight: 600;
                }
                QPushButton#dangerButton {
                    background: #b9414c;
                    color: white;
                    font-weight: 600;
                }
            )");
            return;
        }

        app.setStyleSheet(R"(
            QWidget {
                background-color: #151a22;
                color: #e8edf5;
                font-family: "Segoe UI";
            }
            QLabel { background: transparent; }
            QTabWidget::pane {
                border: 1px solid #344153;
                border-radius: 8px;
            }
            QTabBar::tab {
                background: #202833;
                padding: 10px 18px;
            }
            QTabBar::tab:selected {
                background: #3478f6;
            }
            QFrame#appTile {
                background: #1d2530;
                border: 1px solid #344153;
                border-radius: 12px;
            }
            QFrame#appTile:hover {
                border: 2px solid #3478f6;
            }
            QLabel#tilePhoto {
                background: #252f3c;
                border-radius: 8px;
            }
            QLabel#tileName {
                font-size: 16px;
                font-weight: 600;
            }
            QFrame#downloadBanner {
                background: #1d2530;
                border: 1px solid #344153;
                border-radius: 10px;
            }
            QLabel#downloadBannerLabel {
                background: transparent;
                font-weight: 600;
            }
            QProgressBar#downloadProgressBar {
                background: #252f3c;
                border: 0;
                border-radius: 4px;
            }
            QProgressBar#downloadProgressBar::chunk {
                background: #3478f6;
                border-radius: 4px;
            }
            QPushButton#cancelDownloadButton {
                background: #2a3543;
                color: #e8edf5;
            }
            QLineEdit, QComboBox {
                background: #1d2530;
                border: 1px solid #344153;
                border-radius: 6px;
                padding: 8px;
            }
            QPushButton {
                background: #2a3543;
                border: 0;
                border-radius: 6px;
                padding: 9px 14px;
            }
            QPushButton#primaryButton {
                background: #3478f6;
                color: white;
                font-weight: 600;
            }
            QPushButton#secondaryButton {
                background: #2a3543;
                color: #e8edf5;
                font-weight: 600;
            }
            QPushButton#dangerButton {
                background: #a83c47;
                color: white;
                font-weight: 600;
            }
            QPushButton#primaryButton:disabled {
                color: #8793a3;
                background: #252f3c;
            }
        )");
        };

    auto updateLanguage = [&]() {
        const bool polish = language == "pl";

        cancelDownloadButton->setText(polish ? "Anuluj" : "Cancel");

        tabs->setTabText(0, polish ? "Programy" : "Apps");
        tabs->setTabText(1, polish ? "Ustawienia" : "Settings");
        programsTitle->setText(polish ? "Twoje programy" : "Your apps");
        programsSubtitle->setText(
            polish
            ? "Pobieraj i aktualizuj aplikacje Eagle Studio."
            : "Download and update Eagle Studio apps."
        );
        refreshButton->setText(
            polish ? "Odśwież katalog" : "Refresh catalog"
        );
        tileName->setText("Feather Photo");
        launchButton->setText(polish ? "Uruchom" : "Run");
        const bool running = appProcess && appProcess->state() == QProcess::Running;
        const bool hasInstallAction = !installed || updateAvailable;
        launchButton->setVisible(installed && !running);
        stopButton->setText(polish ? "Zatrzymaj" : "Stop");
        stopButton->setVisible(running);
        downloadButton->setVisible(hasInstallAction);
        downloadButton->setText(installed
            ? (polish ? "Aktualizuj" : "Update")
            : (polish ? "Pobierz" : "Download"));
        uninstallButton->setVisible(installed);
        uninstallButton->setText(polish ? "Odinstaluj" : "Uninstall");
        if (detailsLaunchButton)
        {
            detailsLaunchButton->setText(polish ? "Uruchom" : "Run");
            detailsLaunchButton->setVisible(installed && !running);
        }
        if (detailsStopButton) {
            detailsStopButton->setText(polish ? "Zatrzymaj" : "Stop");
            detailsStopButton->setVisible(running);
        }
        if (detailsInstallButton) {
            detailsInstallButton->setVisible(hasInstallAction);
            detailsInstallButton->setText(installed
                ? (polish ? "Aktualizuj" : "Update")
                : (polish ? "Pobierz i zainstaluj" : "Download and install"));
        }
        if (detailsUninstallButton) {
            detailsUninstallButton->setVisible(installed);
            detailsUninstallButton->setText(polish ? "Odinstaluj" : "Uninstall");
        }
        if (!activeReply) {
            tileStatus->setText(updateAvailable
                ? (polish ? QString("Dostępna aktualizacja %1").arg(latestVersion)
                          : QString("Update available: %1").arg(latestVersion))
                : (installed
                    ? (polish ? QString("Zainstalowany · %1").arg(installedVersion)
                              : QString("Installed · %1").arg(installedVersion))
                    : (polish ? "Gotowy do pobrania" : "Ready to download")));
        }
        if (detailsInstallStatus)
            detailsInstallStatus->setText(installed
                ? (polish ? "Feather Photo jest zainstalowany." : "Feather Photo is installed.")
                : (polish ? "Feather Photo nie jest zainstalowany." : "Feather Photo is not installed."));
        if (detailsVersionLabel) {
            const QString current = installedVersion.isEmpty() ? "—" : installedVersion;
            const QString latest = latestVersion.isEmpty()
                ? (polish ? "sprawdzanie…" : "checking…")
                : latestVersion;
            detailsVersionLabel->setText(polish
                ? QString("Zainstalowana wersja: %1 · najnowsza: %2").arg(current, latest)
                : QString("Installed: %1 · latest: %2").arg(current, latest));
        }
        settingsTitle->setText(polish ? "Ustawienia" : "Settings");
        themeLabel->setText(polish ? "Motyw" : "Theme");
        languageLabel->setText(polish ? "Język" : "Language");
        folderLabel->setText(
            polish
            ? "Folder pobierania instalatorów"
            : "Installer download folder"
        );
        chooseFolderButton->setText(
            polish ? "Wybierz…" : "Browse…"
        );
        themeBox->setItemText(0, polish ? "Ciemny" : "Dark");
        themeBox->setItemText(1, polish ? "Jasny" : "Light");
        themeBox->setItemText(2, polish ? "Systemowy" : "System");
        };

    updateLanguage();
    applyTheme();

    QObject::connect(downloadButton, &QPushButton::clicked, &window, [&]() {
        if (startInstall)
            startInstall();
    });

    QObject::connect(uninstallButton, &QPushButton::clicked, &window, [&]() {
        if (uninstallApp)
            uninstallApp();
    });

    QObject::connect(launchButton, &QPushButton::clicked, &window, [&]() {
        if (launchApp)
            launchApp();
    });

    QObject::connect(stopButton, &QPushButton::clicked, &window, [&]() {
        if (stopApp)
            stopApp();
    });

    QObject::connect(refreshButton, &QPushButton::clicked, &window, [&]() {
#ifdef Q_OS_WIN
        installed = !findFeatherPhotoUninstallCommand().isEmpty();
        installedVersion = installed ? findFeatherPhotoInstalledVersion() : QString{};
        updateInstallUi();
#endif
        if (checkForUpdates)
            checkForUpdates();
    });

    QObject::connect(
        themeBox,
        &QComboBox::currentIndexChanged,
        &window,
        [&](int) {
            settings.setValue("theme", themeBox->currentData());
            applyTheme();
        }
    );

    QObject::connect(
        languageBox,
        &QComboBox::currentIndexChanged,
        &window,
        [&](int) {
            language = languageBox->currentData().toString();
            settings.setValue("language", language);
            updateLanguage();
        }
    );

    QObject::connect(
        folderEdit,
        &QLineEdit::editingFinished,
        &window,
        [&]() {
            settings.setValue("downloadFolder", folderEdit->text());
        }
    );

    QObject::connect(
        chooseFolderButton,
        &QPushButton::clicked,
        &window,
        [&]() {
            const QString folder = QFileDialog::getExistingDirectory(
                &window,
                language == "pl"
                ? "Wybierz folder pobierania"
                : "Choose download folder",
                folderEdit->text()
            );

            if (!folder.isEmpty()) {
                folderEdit->setText(folder);
                settings.setValue("downloadFolder", folder);
            }
        }
    );

    window.show();
    QTimer::singleShot(700, &window, [&]() {
        if (checkForUpdates)
            checkForUpdates();
    });
    return app.exec();
}
