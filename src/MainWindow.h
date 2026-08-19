#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

class CodeEditor;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QWidget;
class QLabel;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void newFile();
    void openFolder();
    void saveFile();
    void saveFileAs();
    void openTreeFile(QTreeWidgetItem *item, int column);
    void closeDocument(int index);
    void runCurrentFile();
    void toggleTerminal();
    void executeTerminalCommand();
    void stopActiveProcess();
    void showCommandPalette();

private:
    void buildInterface();
    void createMenus(QWidget *titleBarLayoutTarget);
    void populateDemoWorkspace();
    void populateFolder(const QString &folderPath);
    void appendFolderItems(QTreeWidgetItem *parent, const QString &folderPath, int depth = 0);
    void openDocument(const QString &name, const QString &contents, const QString &filePath = QString());
    void loadFile(const QString &filePath);
    CodeEditor *currentEditor() const;
    QString documentName(CodeEditor *editor) const;
    QString demoContents(const QString &key) const;
    bool writeDocument(CodeEditor *editor, const QString &path);
    void appendTerminal(const QString &line, const QString &kind = QString());
    void startShellCommand(const QString &command);
    void startProcess(const QString &program, const QStringList &arguments, const QString &displayCommand);
    void setWorkspaceName(const QString &name);

    QTreeWidget *explorer_ = nullptr;
    QTabWidget *documentTabs_ = nullptr;
    QPlainTextEdit *terminal_ = nullptr;
    QLineEdit *terminalInput_ = nullptr;
    QProcess *process_ = nullptr;
    QWidget *terminalPanel_ = nullptr;
    QWidget *editorPanel_ = nullptr;
    QLabel *workspaceLabel_ = nullptr;
    QLabel *branchLabel_ = nullptr;
    QLabel *positionLabel_ = nullptr;
    int untitledCount_ = 0;
    QString workspacePath_;
};
