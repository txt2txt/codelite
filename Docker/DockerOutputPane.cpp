#include "ColoursAndFontsManager.h"
#include "DockerOutputPane.h"
#include "bitmap_loader.h"
#include "clDockerContainer.h"
#include "clDockerEvents.h"
#include "codelite_events.h"
#include "event_notifier.h"
#include "globals.h"
#include "imanager.h"

DockerOutputPane::DockerOutputPane(wxWindow* parent)
    : DockerOutputPaneBase(parent)
{
    m_stc->SetReadOnly(true);
    LexerConf::Ptr_t lexer = ColoursAndFontsManager::Get().GetLexer("text");
    lexer->Apply(m_stc);
    EventNotifier::Get()->Bind(wxEVT_WORKSPACE_CLOSED, [&](wxCommandEvent& event) {
        event.Skip();
        Clear();
    });

    m_styler.reset(new clGenericSTCStyler(m_stc));
    {
        wxArrayString words;
        words.Add("successfully");
        m_styler->AddStyle(words, clGenericSTCStyler::kInfo);
    }
    {
        wxArrayString words;
        words.Add("abort ");
        words.Add("Error response from daemon");
        m_styler->AddStyle(words, clGenericSTCStyler::kError);
    }
    m_toolbar->AddTool(wxID_CLEAR, _("Clear"), clGetManager()->GetStdIcons()->LoadBitmap("clear"));
    m_toolbar->Realize();
    m_toolbar->Bind(wxEVT_TOOL,
                    [&](wxCommandEvent& e) {
                        wxUnusedVar(e);
                        Clear();
                    },
                    wxID_CLEAR);
    m_toolbar->Bind(wxEVT_UPDATE_UI, [&](wxUpdateUIEvent& e) { e.Enable(!m_stc->IsEmpty()); }, wxID_CLEAR);

    m_toolbarContainers->AddTool(wxID_CLEAR, _("Kill Container"),
                                 clGetManager()->GetStdIcons()->LoadBitmap("execute_stop"));
    m_toolbarContainers->AddTool(wxID_CLOSE_ALL, _("Kill All"), clGetManager()->GetStdIcons()->LoadBitmap("clear"));
    m_toolbarContainers->AddSeparator();
    m_toolbarContainers->AddTool(wxID_REFRESH, _("Refresh"),
                                 clGetManager()->GetStdIcons()->LoadBitmap("debugger_restart"));

    m_toolbarContainers->Realize();
    m_toolbarContainers->Bind(wxEVT_TOOL, &DockerOutputPane::OnKillContainer, this, wxID_CLEAR);
    m_toolbarContainers->Bind(wxEVT_UPDATE_UI, &DockerOutputPane::OnKillContainerUI, this, wxID_CLEAR);
    m_toolbarContainers->Bind(wxEVT_TOOL, &DockerOutputPane::OnKillAllContainers, this, wxID_CLOSE_ALL);
    m_toolbarContainers->Bind(wxEVT_UPDATE_UI, &DockerOutputPane::OnKillAllContainersUI, this, wxID_CLOSE_ALL);
    m_toolbarContainers->Bind(wxEVT_TOOL, &DockerOutputPane::OnRefreshContainersView, this, wxID_REFRESH);
    m_notebook->Bind(wxEVT_BOOK_PAGE_CHANGED, [&](wxBookCtrlEvent& event) {
        if(m_notebook->GetPageText(m_notebook->GetSelection()) == _("Containers")) {
            clCommandEvent evt(wxEVT_DOCKER_REFRESH_CONTAINERS_VIEW);
            EventNotifier::Get()->AddPendingEvent(evt);
        }
    });
}

DockerOutputPane::~DockerOutputPane() {}

void DockerOutputPane::Clear()
{
    m_stc->SetReadOnly(false);
    m_stc->ClearAll();
    m_stc->SetReadOnly(true);
}

void DockerOutputPane::AddOutputTextWithEOL(const wxString& msg)
{
    wxString message = msg;
    if(!message.EndsWith("\n")) { message << "\n"; }
    AddOutputTextRaw(message);
}

void DockerOutputPane::AddOutputTextRaw(const wxString& msg)
{
    m_stc->SetReadOnly(false);
    m_stc->AddText(msg);
    m_stc->SetReadOnly(true);
    m_stc->ScrollToEnd();
}

void DockerOutputPane::OnKillContainer(wxCommandEvent& event)
{
    clCommandEvent evt(wxEVT_DOCKER_KILL_CONTAINER);

    wxDataViewItemArray items;
    m_dvListCtrlContainers->GetSelections(items);
    wxArrayString ids;

    // Get the list of ids to kill
    for(int i = 0; i < items.GetCount(); ++i) {
        clDockerContainer* cd =
            reinterpret_cast<clDockerContainer*>(m_dvListCtrlContainers->GetItemData(items.Item(i)));
        ids.Add(cd->GetId());
    }
    evt.SetStrings(ids);
    EventNotifier::Get()->AddPendingEvent(evt);
}

void DockerOutputPane::OnKillContainerUI(wxUpdateUIEvent& event)
{
    event.Enable(m_dvListCtrlContainers->GetSelectedItemsCount());
}

void DockerOutputPane::OnKillAllContainers(wxCommandEvent& event)
{
    clCommandEvent evt(wxEVT_DOCKER_KILL_CONTAINER);
    // Get the list of ids to kill
    wxArrayString ids;
    for(int i = 0; i < m_dvListCtrlContainers->GetItemCount(); ++i) {
        clDockerContainer* cd = reinterpret_cast<clDockerContainer*>(
            m_dvListCtrlContainers->GetItemData(m_dvListCtrlContainers->RowToItem(i)));
        ids.Add(cd->GetId());
    }
    evt.SetStrings(ids);
    EventNotifier::Get()->AddPendingEvent(evt);
}

void DockerOutputPane::OnKillAllContainersUI(wxUpdateUIEvent& event)
{
    event.Enable(m_dvListCtrlContainers->GetItemCount());
}

void DockerOutputPane::OnRefreshContainersView(wxCommandEvent& event)
{
    clCommandEvent evt(wxEVT_DOCKER_REFRESH_CONTAINERS_VIEW);
    EventNotifier::Get()->AddPendingEvent(evt);
}

void DockerOutputPane::SetContainers(const clDockerContainer::Vect_t& containers)
{
    m_containers = containers;
    m_dvListCtrlContainers->DeleteAllItems();
    for(size_t i = 0; i < m_containers.size(); ++i) {
        clDockerContainer& container = m_containers[i];
        wxVector<wxVariant> cols;
        cols.push_back(container.GetId());
        cols.push_back(container.GetImage());
        cols.push_back(container.GetCommand());
        cols.push_back(container.GetCreated());
        cols.push_back(container.GetStatus());
        cols.push_back(container.GetPorts());
        cols.push_back(container.GetName());
        m_dvListCtrlContainers->AppendItem(cols, (wxUIntPtr)&container);
    }
    m_dvListCtrlContainers->GetColumn(0)->SetWidth(-2);
    m_dvListCtrlContainers->GetColumn(1)->SetWidth(-2);
    m_dvListCtrlContainers->GetColumn(2)->SetWidth(-2);
    m_dvListCtrlContainers->GetColumn(3)->SetWidth(-2);
    m_dvListCtrlContainers->GetColumn(4)->SetWidth(-2);
    m_dvListCtrlContainers->GetColumn(5)->SetWidth(-2);
    m_dvListCtrlContainers->GetColumn(6)->SetWidth(-2);
}

void DockerOutputPane::SelectTab(const wxString& label) { m_notebook->SetSelection(m_notebook->GetPageIndex(label)); }
