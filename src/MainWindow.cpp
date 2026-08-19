#include "MainWindow.h"

#include "CodeEditor.h"
#include "SyntaxHighlighter.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QTabBar>
#include <QTabWidget>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {
constexpr int FilePathRole = Qt::UserRole + 1;
constexpr int DemoKeyRole = Qt::UserRole + 2;

class TitleBar final : public QWidget {
public:
    explicit TitleBar(QWidget *parent = nullptr) : QWidget(parent) {
        setObjectName("titleBar");
        setFixedHeight(42);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && !childAt(event->pos())) {
            dragging_ = true;
            offset_ = event->globalPos() - window()->frameGeometry().topLeft();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (dragging_ && (event->buttons() & Qt::LeftButton) && !window()->isMaximized()) {
            window()->move(event->globalPos() - offset_);
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        dragging_ = false;
        QWidget::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && !childAt(event->pos())) {
            window()->isMaximized() ? window()->showNormal() : window()->showMaximized();
            event->accept();
            return;
        }
        QWidget::mouseDoubleClickEvent(event);
    }

private:
    bool dragging_ = false;
    QPoint offset_;
};

QToolButton *toolButton(const QString &text, const QString &tooltip, const QString &name = QString()) {
    auto *button = new QToolButton;
    button->setText(text);
    button->setToolTip(tooltip);
    button->setCursor(Qt::PointingHandCursor);
    button->setAutoRaise(true);
    if (!name.isEmpty()) button->setObjectName(name);
    return button;
}

QFrame *rule(const QString &name = QString()) {
    auto *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    if (!name.isEmpty()) line->setObjectName(name);
    return line;
}

QString shellQuote(const QString &value) {
#ifdef Q_OS_WIN
    QString quoted = value;
    quoted.replace('"', QStringLiteral("\\\""));
    return QStringLiteral("\"") + quoted + QStringLiteral("\"");
#else
    QString quoted = value;
    quoted.replace("'", "'\"'\"'");
    return QStringLiteral("'") + quoted + QStringLiteral("'");
#endif
}

QString displaySize(qint64 bytes) {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
}
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Codingbox");
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setMinimumSize(1040, 680);
    resize(1500, 920);
    buildInterface();

    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(process_, &QProcess::readyReadStandardOutput, this, [this] {
        const QString output = QString::fromLocal8Bit(process_->readAllStandardOutput());
        if (output.isEmpty()) return;
        terminal_->moveCursor(QTextCursor::End);
        terminal_->insertPlainText(output);
        terminal_->moveCursor(QTextCursor::End);
    });
    connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const QString outcome = exitStatus == QProcess::NormalExit
                    ? QString("[process exited with code %1]").arg(exitCode)
                    : QStringLiteral("[process was terminated]");
                appendTerminal(outcome, exitCode == 0 ? "success" : "error");
            });
    connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            appendTerminal("[unable to start process: " + process_->errorString() + "]", "error");
        }
    });

    populateDemoWorkspace();

    openDocument("main.cpp", demoContents("main.cpp"));
    openDocument("app.qss", demoContents("app.qss"));
    documentTabs_->setCurrentIndex(0);
    appendTerminal("Codingbox 0.1.0 — native C++ workspace ready", "muted");
    appendTerminal("Tip: press Ctrl+Shift+P to open the command palette.", "muted");
}

void MainWindow::buildInterface() {
    auto *root = new QWidget;
    root->setObjectName("appRoot");
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *titleBar = new TitleBar(root);
    auto *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(12, 0, 0, 0);
    titleLayout->setSpacing(0);

    auto *mark = new QLabel("<> ");
    mark->setObjectName("brandMark");
    mark->setAlignment(Qt::AlignCenter);
    mark->setFixedSize(28, 28);
    titleLayout->addWidget(mark);

    auto *wordmark = new QLabel("CODINGBOX");
    wordmark->setObjectName("wordmark");
    wordmark->setFixedWidth(112);
    titleLayout->addWidget(wordmark);

    auto *divider = new QFrame;
    divider->setObjectName("titleDivider");
    divider->setFrameShape(QFrame::VLine);
    divider->setFixedHeight(19);
    titleLayout->addWidget(divider);
    titleLayout->addSpacing(8);

    createMenus(titleBar);
    titleLayout->addStretch(1);

    auto *paletteHint = toolButton("⌘  COMMAND", "Open command palette (Ctrl+Shift+P)", "paletteHint");
    paletteHint->setFixedHeight(26);
    connect(paletteHint, &QToolButton::clicked, this, &MainWindow::showCommandPalette);
    titleLayout->addWidget(paletteHint);
    titleLayout->addSpacing(8);

    auto *run = toolButton("▶  RUN", "Run current file (F5)", "runButton");
    run->setFixedHeight(28);
    connect(run, &QToolButton::clicked, this, &MainWindow::runCurrentFile);
    titleLayout->addWidget(run);
    titleLayout->addSpacing(8);

    auto *minimize = toolButton("—", "Minimize", "windowControl");
    auto *maximize = toolButton("□", "Maximize or restore", "windowControl");
    auto *close = toolButton("×", "Close", "windowControlClose");
    minimize->setFixedSize(42, 42);
    maximize->setFixedSize(42, 42);
    close->setFixedSize(42, 42);
    connect(minimize, &QToolButton::clicked, this, &QWidget::showMinimized);
    connect(maximize, &QToolButton::clicked, this, [this] {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(close, &QToolButton::clicked, this, &QWidget::close);
    titleLayout->addWidget(minimize);
    titleLayout->addWidget(maximize);
    titleLayout->addWidget(close);
    rootLayout->addWidget(titleBar);

    auto *mainArea = new QWidget;
    auto *mainLayout = new QHBoxLayout(mainArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *activity = new QFrame;
    activity->setObjectName("activityBar");
    activity->setFixedWidth(56);
    auto *activityLayout = new QVBoxLayout(activity);
    activityLayout->setContentsMargins(0, 10, 0, 8);
    activityLayout->setSpacing(3);
    auto *explorerButton = toolButton("▤", "Explorer", "activityButtonActive");
    auto *searchButton = toolButton("⌕", "Search", "activityButton");
    auto *sourceButton = toolButton("⑂", "Source control", "activityButton");
    auto *debugButton = toolButton("▷", "Run and debug", "activityButton");
    auto *extensionsButton = toolButton("▦", "Extensions", "activityButton");
    for (auto *button : {explorerButton, searchButton, sourceButton, debugButton, extensionsButton}) {
        button->setFixedSize(56, 42);
        activityLayout->addWidget(button);
    }
    activityLayout->addStretch(1);
    auto *terminalButton = toolButton("›_", "Toggle terminal", "activityButton");
    auto *settingsButton = toolButton("⚙", "Settings", "activityButton");
    terminalButton->setFixedSize(56, 42);
    settingsButton->setFixedSize(56, 42);
    connect(terminalButton, &QToolButton::clicked, this, &MainWindow::toggleTerminal);
    connect(settingsButton, &QToolButton::clicked, this, &MainWindow::showCommandPalette);
    activityLayout->addWidget(terminalButton);
    activityLayout->addWidget(settingsButton);
    mainLayout->addWidget(activity);

    auto *explorerPanel = new QFrame;
    explorerPanel->setObjectName("explorerPanel");
    explorerPanel->setMinimumWidth(220);
    explorerPanel->setMaximumWidth(310);
    auto *explorerLayout = new QVBoxLayout(explorerPanel);
    explorerLayout->setContentsMargins(0, 0, 0, 0);
    explorerLayout->setSpacing(0);

    auto *explorerHeading = new QWidget;
    explorerHeading->setObjectName("sideHeading");
    explorerHeading->setFixedHeight(39);
    auto *headingLayout = new QHBoxLayout(explorerHeading);
    headingLayout->setContentsMargins(14, 0, 8, 0);
    auto *heading = new QLabel("EXPLORER");
    heading->setObjectName("sectionLabel");
    headingLayout->addWidget(heading);
    headingLayout->addStretch(1);
    auto *openFolderButton = toolButton("⌑", "Open folder", "sideAction");
    auto *newFileButton = toolButton("+", "New file", "sideAction");
    openFolderButton->setFixedSize(25, 25);
    newFileButton->setFixedSize(25, 25);
    connect(openFolderButton, &QToolButton::clicked, this, &MainWindow::openFolder);
    connect(newFileButton, &QToolButton::clicked, this, &MainWindow::newFile);
    headingLayout->addWidget(openFolderButton);
    headingLayout->addWidget(newFileButton);
    explorerLayout->addWidget(explorerHeading);
    explorerLayout->addWidget(rule());

    auto *workspaceRow = new QWidget;
    workspaceRow->setObjectName("workspaceRow");
    workspaceRow->setFixedHeight(34);
    auto *workspaceLayout = new QHBoxLayout(workspaceRow);
    workspaceLayout->setContentsMargins(13, 0, 9, 0);
    auto *chevron = new QLabel("⌄");
    chevron->setObjectName("treeChevron");
    workspaceLabel_ = new QLabel("GETTING-STARTED");
    workspaceLabel_->setObjectName("workspaceLabel");
    workspaceLayout->addWidget(chevron);
    workspaceLayout->addSpacing(5);
    workspaceLayout->addWidget(workspaceLabel_);
    workspaceLayout->addStretch();
    explorerLayout->addWidget(workspaceRow);

    explorer_ = new QTreeWidget;
    explorer_->setObjectName("fileTree");
    explorer_->setHeaderHidden(true);
    explorer_->setIndentation(16);
    explorer_->setAnimated(true);
    explorer_->setExpandsOnDoubleClick(true);
    explorer_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(explorer_, &QTreeWidget::itemActivated, this, &MainWindow::openTreeFile);
    connect(explorer_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item) {
        if (item && item->childCount() == 0) openTreeFile(item, 0);
    });
    explorerLayout->addWidget(explorer_, 1);

    auto *outline = new QLabel("OUTLINE\n\n  No symbols selected");
    outline->setObjectName("outlinePanel");
    outline->setMinimumHeight(86);
    outline->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    explorerLayout->addWidget(rule());
    explorerLayout->addWidget(outline);
    mainLayout->addWidget(explorerPanel);

    auto *workbench = new QWidget;
    workbench->setObjectName("workbench");
    auto *workbenchLayout = new QVBoxLayout(workbench);
    workbenchLayout->setContentsMargins(0, 0, 0, 0);
    workbenchLayout->setSpacing(0);

    auto *crumbs = new QWidget;
    crumbs->setObjectName("breadcrumbs");
    crumbs->setFixedHeight(29);
    auto *crumbLayout = new QHBoxLayout(crumbs);
    crumbLayout->setContentsMargins(15, 0, 12, 0);
    auto *crumbHome = new QLabel("⌂  getting-started");
    crumbHome->setObjectName("crumbMuted");
    auto *crumbSep = new QLabel("/  src  /");
    crumbSep->setObjectName("crumbMuted");
    auto *crumbFile = new QLabel("main.cpp");
    crumbFile->setObjectName("crumbFile");
    crumbLayout->addWidget(crumbHome);
    crumbLayout->addSpacing(8);
    crumbLayout->addWidget(crumbSep);
    crumbLayout->addSpacing(8);
    crumbLayout->addWidget(crumbFile);
    crumbLayout->addStretch(1);
    auto *splitIcon = toolButton("▥", "Split editor", "crumbAction");
    splitIcon->setFixedSize(28, 28);
    crumbLayout->addWidget(splitIcon);
    workbenchLayout->addWidget(crumbs);

    auto *panes = new QSplitter(Qt::Vertical);
    panes->setObjectName("editorSplitter");
    panes->setHandleWidth(1);
    panes->setChildrenCollapsible(false);

    editorPanel_ = new QWidget;
    editorPanel_->setObjectName("editorPanel");
    auto *editorLayout = new QVBoxLayout(editorPanel_);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    documentTabs_ = new QTabWidget;
    documentTabs_->setObjectName("documentTabs");
    documentTabs_->setTabsClosable(true);
    documentTabs_->setMovable(true);
    documentTabs_->setDocumentMode(true);
    documentTabs_->tabBar()->setExpanding(false);
    documentTabs_->tabBar()->setElideMode(Qt::ElideRight);
    connect(documentTabs_, &QTabWidget::tabCloseRequested, this, &MainWindow::closeDocument);
    connect(documentTabs_, &QTabWidget::currentChanged, this, [this, crumbFile](int) {
        if (auto *editor = currentEditor()) {
            crumbFile->setText(documentName(editor));
            const auto cursor = editor->textCursor();
            positionLabel_->setText(QString("Ln %1, Col %2")
                                    .arg(cursor.blockNumber() + 1)
                                    .arg(cursor.columnNumber() + 1));
        }
    });
    editorLayout->addWidget(documentTabs_);
    panes->addWidget(editorPanel_);

    terminalPanel_ = new QFrame;
    terminalPanel_->setObjectName("terminalPanel");
    terminalPanel_->setMinimumHeight(146);
    auto *terminalLayout = new QVBoxLayout(terminalPanel_);
    terminalLayout->setContentsMargins(0, 0, 0, 0);
    terminalLayout->setSpacing(0);
    auto *terminalHeader = new QWidget;
    terminalHeader->setObjectName("terminalHeader");
    terminalHeader->setFixedHeight(35);
    auto *terminalHeaderLayout = new QHBoxLayout(terminalHeader);
    terminalHeaderLayout->setContentsMargins(12, 0, 8, 0);
    auto *terminalName = new QLabel("TERMINAL");
    terminalName->setObjectName("terminalName");
    auto *shell = new QLabel("zsh");
    shell->setObjectName("shellName");
    terminalHeaderLayout->addWidget(terminalName);
    terminalHeaderLayout->addSpacing(15);
    terminalHeaderLayout->addWidget(shell);
    terminalHeaderLayout->addStretch(1);
    auto *clear = toolButton("⌫", "Clear terminal", "terminalAction");
    auto *stop = toolButton("■", "Stop active process", "terminalAction");
    auto *hideTerminal = toolButton("×", "Hide terminal", "terminalAction");
    clear->setFixedSize(28, 28);
    stop->setFixedSize(28, 28);
    hideTerminal->setFixedSize(28, 28);
    connect(clear, &QToolButton::clicked, this, [this] { terminal_->clear(); });
    connect(stop, &QToolButton::clicked, this, &MainWindow::stopActiveProcess);
    connect(hideTerminal, &QToolButton::clicked, this, &MainWindow::toggleTerminal);
    terminalHeaderLayout->addWidget(clear);
    terminalHeaderLayout->addWidget(stop);
    terminalHeaderLayout->addWidget(hideTerminal);
    terminalLayout->addWidget(terminalHeader);
    terminalLayout->addWidget(rule());

    terminal_ = new QPlainTextEdit;
    terminal_->setObjectName("terminalOutput");
    terminal_->setReadOnly(true);
    terminal_->setMaximumBlockCount(500);
    terminalLayout->addWidget(terminal_, 1);
    terminalInput_ = new QLineEdit;
    terminalInput_->setObjectName("terminalInput");
    terminalInput_->setPlaceholderText("$  Type a command and press Enter");
    connect(terminalInput_, &QLineEdit::returnPressed, this, &MainWindow::executeTerminalCommand);
    terminalLayout->addWidget(terminalInput_);
    panes->addWidget(terminalPanel_);
    panes->setStretchFactor(0, 8);
    panes->setStretchFactor(1, 2);
    panes->setSizes({610, 190});
    workbenchLayout->addWidget(panes, 1);
    mainLayout->addWidget(workbench, 1);
    rootLayout->addWidget(mainArea, 1);

    auto *status = new QWidget;
    status->setObjectName("statusBar");
    status->setFixedHeight(24);
    auto *statusLayout = new QHBoxLayout(status);
    statusLayout->setContentsMargins(10, 0, 10, 0);
    auto *remote = new QLabel("⌘  main*");
    remote->setObjectName("statusAccent");
    branchLabel_ = new QLabel("  ◉  workspace ready");
    branchLabel_->setObjectName("statusText");
    statusLayout->addWidget(remote);
    statusLayout->addWidget(branchLabel_);
    statusLayout->addStretch(1);
    auto *spaces = new QLabel("Spaces: 2");
    auto *encoding = new QLabel("UTF-8");
    auto *language = new QLabel("C++");
    positionLabel_ = new QLabel("Ln 1, Col 1");
    for (auto *label : {spaces, encoding, language, positionLabel_}) {
        label->setObjectName("statusText");
        statusLayout->addWidget(label);
        statusLayout->addSpacing(15);
    }
    rootLayout->addWidget(status);

    setCentralWidget(root);

    auto *newAction = new QAction("New File", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    addAction(newAction);
    auto *openAction = new QAction("Open Folder…", this);
    openAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(openAction, &QAction::triggered, this, &MainWindow::openFolder);
    addAction(openAction);
    auto *saveAction = new QAction("Save", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    addAction(saveAction);
    auto *runAction = new QAction("Run Current File", this);
    runAction->setShortcut(QKeySequence("F5"));
    connect(runAction, &QAction::triggered, this, &MainWindow::runCurrentFile);
    addAction(runAction);
    auto *terminalAction = new QAction("Toggle Terminal", this);
    terminalAction->setShortcut(QKeySequence("Ctrl+`"));
    connect(terminalAction, &QAction::triggered, this, &MainWindow::toggleTerminal);
    addAction(terminalAction);
    auto *commandAction = new QAction("Command Palette", this);
    commandAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
    connect(commandAction, &QAction::triggered, this, &MainWindow::showCommandPalette);
    addAction(commandAction);

    setStyleSheet(R"(
        * { outline: 0; border-radius: 0; }
        QMainWindow, #appRoot { background: #071b40; color: #dcecff; }
        #titleBar { background: #061733; border-bottom: 1px solid #2b84c6; }
        #brandMark { background: #1a91d5; color: #03142f; font-family: "JetBrains Mono", monospace; font-size: 11px; font-weight: 800; }
        #wordmark { color: #e7f6ff; font-family: "JetBrains Mono", monospace; font-size: 12px; font-weight: 700; letter-spacing: 1px; padding-left: 8px; }
        #titleDivider { color: #2a6193; background: #2a6193; max-width: 1px; }
        QMenuBar { background: transparent; color: #95c8ef; font-size: 11px; }
        QMenuBar::item { padding: 5px 9px; background: transparent; }
        QMenuBar::item:selected { background: #103667; color: #ffffff; }
        QMenu { background: #092452; border: 1px solid #4aa6e2; padding: 4px; color: #dcecff; }
        QMenu::item { padding: 7px 28px 7px 12px; }
        QMenu::item:selected { background: #135c99; }
        #paletteHint { background: #0a2859; border: 1px solid #286fa7; color: #9ccfee; font-size: 10px; padding: 0 8px; }
        #paletteHint:hover { background: #123b72; border-color: #66c6fa; }
        #runButton { background: #1a92d0; border: 1px solid #72d1fc; color: #021938; font-weight: 800; font-size: 10px; padding: 0 10px; }
        #runButton:hover { background: #59bdf0; }
        #windowControl { background: transparent; color: #85b9df; font-size: 16px; }
        #windowControl:hover { background: #17406f; color: #ffffff; }
        #windowControlClose { background: transparent; color: #85b9df; font-size: 19px; }
        #windowControlClose:hover { background: #d8495e; color: #ffffff; }
        #activityBar { background: #061735; border-right: 1px solid #2b84c6; }
        #activityButton, #activityButtonActive { color: #71a9d2; font-family: "JetBrains Mono", monospace; font-size: 20px; }
        #activityButton:hover { background: #0d3062; color: #cbeaff; }
        #activityButtonActive { background: #0d3062; color: #7bd4ff; border-left: 3px solid #5fd3ff; }
        #explorerPanel { background: #081d43; border-right: 1px solid #2b84c6; }
        #sideHeading { background: #081d43; }
        #sectionLabel { color: #9cc9ea; font-size: 10px; font-weight: 700; letter-spacing: 1px; }
        #sideAction { color: #85bcdf; font-size: 17px; }
        #sideAction:hover { background: #123563; color: #e3f5ff; }
        QFrame[frameShape="4"] { color: #215f94; background: #215f94; max-height: 1px; }
        #workspaceRow { background: #0a2450; }
        #treeChevron { color: #5baee1; font-size: 15px; }
        #workspaceLabel { color: #b8e2fa; font-size: 11px; font-weight: 700; letter-spacing: .5px; }
        #fileTree { background: #081d43; border: 0; color: #9dc6e6; font-size: 11px; padding: 4px 5px; }
        #fileTree::item { height: 25px; padding-left: 3px; border: 0; }
        #fileTree::item:hover { background: #0e3262; color: #e4f5ff; }
        #fileTree::item:selected { background: #145c94; color: #ffffff; }
        #fileTree::branch:has-children:closed { image: none; border-image: none; }
        #outlinePanel { background: #071a3c; color: #5783a9; font-size: 10px; padding: 11px 14px; line-height: 18px; }
        #workbench { background: #0a2350; }
        #breadcrumbs { background: #0b2757; border-bottom: 1px solid #235d91; }
        #crumbMuted { color: #699dca; font-size: 10px; }
        #crumbFile { color: #c9e8ff; font-size: 10px; }
        #crumbAction { color: #78afd4; font-size: 14px; }
        #crumbAction:hover { background: #154171; color: #e5f5ff; }
        #editorPanel { background: #0a2350; }
        #documentTabs::pane { border: 0; top: 0; }
        #documentTabs > QTabBar { background: #081d43; border-bottom: 1px solid #2b84c6; }
        #documentTabs QTabBar::tab { background: #081d43; color: #78a7cd; border-right: 1px solid #215886; border-bottom: 2px solid transparent; min-width: 118px; padding: 10px 14px 9px 14px; font-size: 11px; }
        #documentTabs QTabBar::tab:selected { background: #0a2350; color: #e0f2ff; border-bottom: 2px solid #5fd3ff; }
        #documentTabs QTabBar::tab:hover:!selected { background: #0d2d59; color: #bfe1f7; }
        #documentTabs QTabBar::close-button { image: none; subcontrol-position: right; padding: 1px; }
        #documentTabs QTabBar::close-button:hover { background: #d84e60; }
        CodeEditor { background: #0a2350; color: #d9efff; border: 0; selection-background-color: #1c5f98; selection-color: #ffffff; padding: 8px 22px 20px 8px; font-family: "JetBrains Mono", "Cascadia Mono", "Menlo", monospace; font-size: 12px; line-height: 1.35; }
        CodeEditor QScrollBar:vertical { background: #081d43; width: 10px; margin: 0; }
        CodeEditor QScrollBar::handle:vertical { background: #245d91; min-height: 25px; }
        CodeEditor QScrollBar::handle:vertical:hover { background: #4399ce; }
        CodeEditor QScrollBar::add-line:vertical, CodeEditor QScrollBar::sub-line:vertical { height: 0; }
        #editorSplitter::handle { background: #3c91c5; height: 1px; }
        #editorSplitter::handle:hover { background: #70d3ff; }
        #terminalPanel { background: #061a3d; border-top: 0; }
        #terminalHeader { background: #081d43; }
        #terminalName { color: #b9e6ff; font-size: 10px; font-weight: 700; letter-spacing: 1px; }
        #shellName { background: #10345f; color: #7fc7ef; border: 1px solid #286493; padding: 2px 6px; font-size: 9px; }
        #terminalAction { color: #77add1; font-size: 16px; }
        #terminalAction:hover { background: #17426f; color: #ffffff; }
        #terminalOutput { background: #061a3d; color: #b5d7ec; border: 0; padding: 8px 12px; font-family: "JetBrains Mono", "Cascadia Mono", monospace; font-size: 11px; selection-background-color: #175e97; }
        #terminalInput { background: #081f48; color: #cbeaff; border: 0; border-top: 1px solid #245d91; min-height: 28px; padding: 0 12px; font-family: "JetBrains Mono", "Cascadia Mono", monospace; font-size: 11px; }
        #terminalInput:focus { border-top-color: #66cfff; }
        #statusBar { background: #126b9f; border-top: 1px solid #6bd1ff; }
        #statusAccent { color: #effbff; font-size: 10px; font-weight: 700; }
        #statusText { color: #d0ecff; font-size: 10px; }
        QDialog { background: #081d43; border: 1px solid #65cfff; }
        QDialog QLabel { color: #d9efff; }
        #commandInput { background: #0b2a5c; border: 1px solid #459bd0; color: #e7f6ff; min-height: 33px; padding: 0 10px; font-size: 12px; }
        #commandInput:focus { border-color: #76d7ff; }
        QPushButton { background: #12598f; border: 1px solid #60c9f5; color: #e4f5ff; min-height: 27px; padding: 0 11px; }
        QPushButton:hover { background: #1a77b8; }
    )");
}

void MainWindow::createMenus(QWidget *titleBarLayoutTarget) {
    auto *layout = qobject_cast<QHBoxLayout *>(titleBarLayoutTarget->layout());
    auto *menuBar = new QMenuBar(titleBarLayoutTarget);
    menuBar->setNativeMenuBar(false);

    auto *file = menuBar->addMenu("File");
    auto *fileNew = file->addAction("New File");
    fileNew->setShortcut(QKeySequence::New);
    connect(fileNew, &QAction::triggered, this, &MainWindow::newFile);
    auto *fileOpen = file->addAction("Open Folder…");
    fileOpen->setShortcut(QKeySequence("Ctrl+O"));
    connect(fileOpen, &QAction::triggered, this, &MainWindow::openFolder);
    file->addSeparator();
    auto *fileSave = file->addAction("Save");
    fileSave->setShortcut(QKeySequence::Save);
    connect(fileSave, &QAction::triggered, this, &MainWindow::saveFile);
    auto *fileSaveAs = file->addAction("Save As…");
    fileSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(fileSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    file->addSeparator();
    auto *quit = file->addAction("Quit Codingbox");
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    auto *edit = menuBar->addMenu("Edit");
    auto *undo = edit->addAction("Undo");
    undo->setShortcut(QKeySequence::Undo);
    connect(undo, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->undo(); });
    auto *redo = edit->addAction("Redo");
    redo->setShortcut(QKeySequence::Redo);
    connect(redo, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->redo(); });
    edit->addSeparator();
    auto *find = edit->addAction("Find");
    find->setShortcut(QKeySequence::Find);
    connect(find, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->setFocus(); });

    auto *selection = menuBar->addMenu("Selection");
    auto *selectAll = selection->addAction("Select All");
    selectAll->setShortcut(QKeySequence::SelectAll);
    connect(selectAll, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->selectAll(); });

    auto *runMenu = menuBar->addMenu("Run");
    auto *run = runMenu->addAction("Run Current File");
    run->setShortcut(QKeySequence("F5"));
    connect(run, &QAction::triggered, this, &MainWindow::runCurrentFile);
    auto *toggle = runMenu->addAction("Toggle Terminal");
    toggle->setShortcut(QKeySequence("Ctrl+`"));
    connect(toggle, &QAction::triggered, this, &MainWindow::toggleTerminal);

    auto *view = menuBar->addMenu("View");
    auto *palette = view->addAction("Command Palette…");
    palette->setShortcut(QKeySequence("Ctrl+Shift+P"));
    connect(palette, &QAction::triggered, this, &MainWindow::showCommandPalette);
    auto *terminal = view->addAction("Terminal");
    connect(terminal, &QAction::triggered, this, &MainWindow::toggleTerminal);

    auto *help = menuBar->addMenu("Help");
    auto *about = help->addAction("About Codingbox");
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::about(this, "About Codingbox",
                           "Codingbox 0.1.0\n\nA native C++ desktop coding workspace built with Qt Widgets.\n\nLicensed under BSD-2-Clause.");
    });

    layout->addWidget(menuBar);
}

void MainWindow::populateDemoWorkspace() {
    explorer_->clear();
    auto *src = new QTreeWidgetItem(explorer_, {"▾  src"});
    src->setExpanded(true);
    auto *main = new QTreeWidgetItem(src, {"◫  main.cpp"});
    main->setData(0, DemoKeyRole, "main.cpp");
    auto *theme = new QTreeWidgetItem(src, {"◫  app.qss"});
    theme->setData(0, DemoKeyRole, "app.qss");
    auto *components = new QTreeWidgetItem(src, {"▸  components"});
    components->addChild(new QTreeWidgetItem({"    empty"}));
    auto *cmake = new QTreeWidgetItem(explorer_, {"◇  CMakeLists.txt"});
    cmake->setData(0, DemoKeyRole, "CMakeLists.txt");
    auto *readme = new QTreeWidgetItem(explorer_, {"◇  README.md"});
    readme->setData(0, DemoKeyRole, "README.md");
    auto *ignore = new QTreeWidgetItem(explorer_, {"◇  .gitignore"});
    ignore->setData(0, DemoKeyRole, ".gitignore");
    explorer_->expandItem(src);
    setWorkspaceName("GETTING-STARTED");
}

void MainWindow::openFolder() {
    const QString folder = QFileDialog::getExistingDirectory(this, "Open folder in Codingbox");
    if (folder.isEmpty()) return;
    workspacePath_ = folder;
    populateFolder(folder);
    appendTerminal(QString("Opened workspace: %1").arg(folder), "muted");
}

void MainWindow::populateFolder(const QString &folderPath) {
    explorer_->clear();
    appendFolderItems(nullptr, folderPath);
    setWorkspaceName(QFileInfo(folderPath).fileName().toUpper());
}

void MainWindow::appendFolderItems(QTreeWidgetItem *parent, const QString &folderPath, int depth) {
    QDir directory(folderPath);
    const QFileInfoList entries = directory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                                          QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &entry : entries) {
        const bool isDirectory = entry.isDir();
        auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(explorer_);
        item->setText(0, QString(isDirectory ? "▸  %1" : "◫  %1").arg(entry.fileName()));
        item->setData(0, FilePathRole, entry.absoluteFilePath());
        if (isDirectory && depth < 3) {
            appendFolderItems(item, entry.absoluteFilePath(), depth + 1);
            item->setExpanded(depth == 0);
        }
    }
}

void MainWindow::newFile() {
    ++untitledCount_;
    openDocument(QString("untitled-%1.cpp").arg(untitledCount_),
                 "#include <iostream>\n\nint main() {\n  // Start building in Codingbox.\n  return 0;\n}\n");
}

void MainWindow::openTreeFile(QTreeWidgetItem *item, int) {
    if (!item || item->childCount() > 0) return;
    const QString filePath = item->data(0, FilePathRole).toString();
    const QString key = item->data(0, DemoKeyRole).toString();
    if (!filePath.isEmpty() && QFileInfo(filePath).isDir()) {
        item->setExpanded(!item->isExpanded());
        return;
    }
    if (!filePath.isEmpty()) loadFile(filePath);
    else if (!key.isEmpty()) openDocument(key, demoContents(key));
}

void MainWindow::loadFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Could not open file", QString("Codingbox could not read:\n%1").arg(filePath));
        return;
    }
    openDocument(QFileInfo(filePath).fileName(), QString::fromUtf8(file.readAll()), filePath);
}

void MainWindow::openDocument(const QString &name, const QString &contents, const QString &filePath) {
    for (int index = 0; index < documentTabs_->count(); ++index) {
        auto *existing = qobject_cast<CodeEditor *>(documentTabs_->widget(index));
        if (existing && ((!filePath.isEmpty() && existing->property("filePath").toString() == filePath) ||
                         (filePath.isEmpty() && documentName(existing) == name))) {
            documentTabs_->setCurrentIndex(index);
            return;
        }
    }

    auto *editor = new CodeEditor;
    editor->setPlainText(contents);
    editor->setProperty("filePath", filePath);
    editor->setProperty("displayName", name);
    new SyntaxHighlighter(editor->document());
    editor->document()->setModified(false);
    const int index = documentTabs_->addTab(editor, "◫  " + name);
    documentTabs_->setCurrentIndex(index);
    connect(editor->document(), &QTextDocument::modificationChanged, this,
            [this, editor](bool modified) {
                const int tabIndex = documentTabs_->indexOf(editor);
                if (tabIndex >= 0) {
                    documentTabs_->setTabText(tabIndex,
                        QString("◫  %1%2").arg(documentName(editor), modified ? " •" : ""));
                }
            });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, [this, editor] {
        if (editor != currentEditor()) return;
        const auto cursor = editor->textCursor();
        positionLabel_->setText(QString("Ln %1, Col %2")
                                .arg(cursor.blockNumber() + 1)
                                .arg(cursor.columnNumber() + 1));
    });
}

CodeEditor *MainWindow::currentEditor() const {
    return qobject_cast<CodeEditor *>(documentTabs_->currentWidget());
}

QString MainWindow::documentName(CodeEditor *editor) const {
    return editor ? editor->property("displayName").toString() : QString();
}

bool MainWindow::writeDocument(CodeEditor *editor, const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Could not save file", QString("Codingbox could not write:\n%1").arg(path));
        return false;
    }
    file.write(editor->toPlainText().toUtf8());
    file.flush();
    editor->setProperty("filePath", path);
    editor->setProperty("displayName", QFileInfo(path).fileName());
    editor->document()->setModified(false);
    const int tabIndex = documentTabs_->indexOf(editor);
    if (tabIndex >= 0) documentTabs_->setTabText(tabIndex, "◫  " + documentName(editor));
    appendTerminal(QString("Saved %1  (%2)").arg(path, displaySize(file.size())), "muted");
    return true;
}

void MainWindow::saveFile() {
    auto *editor = currentEditor();
    if (!editor) return;
    const QString path = editor->property("filePath").toString();
    if (path.isEmpty()) {
        saveFileAs();
        return;
    }
    writeDocument(editor, path);
}

void MainWindow::saveFileAs() {
    auto *editor = currentEditor();
    if (!editor) return;
    const QString proposed = editor->property("filePath").toString().isEmpty()
                                 ? documentName(editor)
                                 : editor->property("filePath").toString();
    const QString path = QFileDialog::getSaveFileName(this, "Save file", proposed,
        "Source files (*.cpp *.c *.h *.hpp *.py *.js *.ts *.html *.css);;All files (*)");
    if (!path.isEmpty()) writeDocument(editor, path);
}

void MainWindow::closeDocument(int index) {
    auto *editor = qobject_cast<CodeEditor *>(documentTabs_->widget(index));
    if (!editor) return;
    if (editor->document()->isModified()) {
        const auto result = QMessageBox::question(this, "Unsaved changes",
            QString("Save changes to %1?").arg(documentName(editor)),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (result == QMessageBox::Cancel) return;
        if (result == QMessageBox::Save) {
            documentTabs_->setCurrentIndex(index);
            saveFile();
            if (editor->document()->isModified()) return;
        }
    }
    documentTabs_->removeTab(index);
    editor->deleteLater();
    if (documentTabs_->count() == 0) newFile();
}

void MainWindow::runCurrentFile() {
    auto *editor = currentEditor();
    if (!editor) return;

    if (editor->document()->isModified()) saveFile();
    if (editor->document()->isModified()) return;
    const QString sourcePath = editor->property("filePath").toString();
    if (sourcePath.isEmpty()) {
        appendTerminal("Save the file before running it.", "error");
        return;
    }

    if (!terminalPanel_->isVisible()) toggleTerminal();
    const QString suffix = QFileInfo(sourcePath).suffix().toLower();
    const QString source = shellQuote(sourcePath);

    if (suffix == "cpp" || suffix == "cxx" || suffix == "cc" || suffix == "c") {
        QString executable = QDir(QDir::tempPath()).filePath(
            "codingbox-" + QFileInfo(sourcePath).completeBaseName());
#ifdef Q_OS_WIN
        executable += ".exe";
#endif
        const QString compiler = suffix == "c" ? "gcc" : "g++";
        const QString standard = suffix == "c" ? "-std=c17" : "-std=c++17";
        startShellCommand(QString("%1 %2 -Wall %3 -o %4 && %4")
            .arg(compiler, standard, source, shellQuote(executable)));
    } else if (suffix == "py") {
#ifdef Q_OS_WIN
        startShellCommand("py " + source);
#else
        startShellCommand("python3 " + source);
#endif
    } else if (suffix == "js" || suffix == "mjs") {
        startShellCommand("node " + source);
    } else if (suffix == "sh") {
        startShellCommand("sh " + source);
    } else {
        appendTerminal(QString("No runner is configured for .%1 files.").arg(suffix), "error");
    }
}

void MainWindow::toggleTerminal() {
    terminalPanel_->setVisible(!terminalPanel_->isVisible());
    if (terminalPanel_->isVisible()) terminalInput_->setFocus();
}

void MainWindow::appendTerminal(const QString &line, const QString &) {
    terminal_->appendPlainText(line);
    terminal_->moveCursor(QTextCursor::End);
}

void MainWindow::startProcess(const QString &program, const QStringList &arguments,
                              const QString &displayCommand) {
    if (process_->state() != QProcess::NotRunning) {
        appendTerminal("A process is already running. Stop it before starting another command.", "error");
        return;
    }
    process_->setWorkingDirectory(workspacePath_.isEmpty() ? QDir::currentPath() : workspacePath_);
    appendTerminal(displayCommand);
    process_->start(program, arguments);
}

void MainWindow::startShellCommand(const QString &command) {
#ifdef Q_OS_WIN
    startProcess("cmd.exe", {"/C", command}, "$ " + command);
#else
    startProcess("/bin/sh", {"-lc", command}, "$ " + command);
#endif
}

void MainWindow::stopActiveProcess() {
    if (process_->state() == QProcess::NotRunning) {
        appendTerminal("No active process to stop.", "muted");
        return;
    }
    appendTerminal("[stopping active process…]", "muted");
    process_->terminate();
}

void MainWindow::executeTerminalCommand() {
    const QString command = terminalInput_->text().trimmed();
    if (command.isEmpty()) return;
    terminalInput_->clear();
    if (command == "clear") {
        terminal_->clear();
        return;
    }
    if (command == "help") {
        appendTerminal("Runs local shell commands. Built-ins: help, clear, run, stop.", "muted");
        return;
    }
    if (command == "run") {
        runCurrentFile();
        return;
    }
    if (command == "stop") {
        stopActiveProcess();
        return;
    }
    startShellCommand(command);
}

void MainWindow::showCommandPalette() {
    QDialog dialog(this);
    dialog.setWindowTitle("Command Palette");
    dialog.setModal(true);
    dialog.setFixedSize(560, 214);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 18, 18, 18);
    auto *title = new QLabel("COMMAND PALETTE");
    title->setStyleSheet("color: #84c9ef; font-size: 10px; font-weight: 700; letter-spacing: 1px;");
    auto *input = new QLineEdit;
    input->setObjectName("commandInput");
    input->setPlaceholderText("Type a command…");
    auto *hint = new QLabel("New File     Open Folder     Save File     Run Current File     Toggle Terminal");
    hint->setStyleSheet("color: #6d9dc1; font-size: 10px; padding: 4px 0;");
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    auto *execute = buttons->addButton("Execute", QDialogButtonBox::AcceptRole);
    layout->addWidget(title);
    layout->addSpacing(5);
    layout->addWidget(input);
    layout->addWidget(hint);
    layout->addStretch();
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(execute, &QPushButton::clicked, &dialog, &QDialog::accept);
    input->setFocus();
    if (dialog.exec() != QDialog::Accepted) return;

    const QString command = input->text().trimmed().toLower();
    if (command.contains("new")) newFile();
    else if (command.contains("open")) openFolder();
    else if (command.contains("save")) saveFile();
    else if (command.contains("run")) runCurrentFile();
    else if (command.contains("terminal")) toggleTerminal();
}

void MainWindow::setWorkspaceName(const QString &name) {
    workspaceLabel_->setText(name);
    branchLabel_->setText("  ◉  " + name.toLower() + " ready");
}

QString MainWindow::demoContents(const QString &key) const {
    if (key == "main.cpp") return R"(#include <iostream>
#include <string>

struct Session {
  std::string name;
  bool ready = true;
};

int main() {
  Session workspace{"codingbox"};

  std::cout << "Welcome to " << workspace.name << "\n";
  std::cout << "Build something focused." << std::endl;

  return workspace.ready ? 0 : 1;
}
)";
    if (key == "app.qss") return R"(QMainWindow {
  background: #071b40;
  color: #dcecff;
}

QPlainTextEdit {
  background: #0a2350;
  border: 0;
  font-family: "JetBrains Mono";
}
)";
    if (key == "CMakeLists.txt") return R"(cmake_minimum_required(VERSION 3.21)
project(Codingbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(Codingbox src/main.cpp)
target_link_libraries(Codingbox PRIVATE Qt6::Widgets)
)";
    if (key == "README.md") return R"(# Codingbox

A focused native desktop IDE.

- C++ and Qt Widgets
- Flat, high-contrast blue interface
- Local file editing and native dialogs
- Integrated workspace, terminal, and run panel
)";
    if (key == ".gitignore") return "build/\n*.user\n.DS_Store\n";
    return {};
}
