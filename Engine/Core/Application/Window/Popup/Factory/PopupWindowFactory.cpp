#include "PopupWindowFactory.h"

NanamiEngine::Core::PopupWindow::PopupWindowFactory& NanamiEngine::Core::PopupWindow::PopupWindowFactory::Instance()
{
    static PopupWindowFactory instance;
    return instance;
}
