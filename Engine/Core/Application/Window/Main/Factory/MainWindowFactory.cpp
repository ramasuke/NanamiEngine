#include "MainWindowFactory.h"

NanamiEngine::Core::MainWindow::MainWindowFactory& NanamiEngine::Core::MainWindow::MainWindowFactory::Instance()
{
    static MainWindowFactory instance;
    return instance;
}
