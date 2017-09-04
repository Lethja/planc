#include "main.h"

IMPLEMENT_APP(WebApp)

bool WebApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    webFrame *frame = new webFrame(m_url);
    frame->Show();

    return true;
}

wxString procUrl(wxString url)
{//Fix any errors in URL before returning to webView
	if(url.Find("://") == wxNOT_FOUND)
	{
		wxString rr = "http://" + url;
		return rr;
	}
	return url;
}

webFrame::webFrame(const wxString& url) : wxFrame(NULL, wxID_ANY, url)
{
	{//ints not needed after size has been set
		int x, y;
		wxDisplaySize(&x,&y);
		if(x > 320 && y > 180)
			this->SetSize(x/2,y/2);
	}
	this->SetSizeHints(wxSize(320,180), wxDefaultSize);

	m_menu = new wxMenuBar(0);
	file = new wxMenu();
	wxMenuItem* fnew;
	fnew = new wxMenuItem( file, wxID_NEW, wxString(wxT("&New")), wxEmptyString, wxITEM_NORMAL);
	file->Append(fnew);

	file->AppendSeparator();

	wxMenuItem* exit;
	exit = new wxMenuItem( file, wxID_EXIT, wxString(wxT("&Exit")), wxEmptyString, wxITEM_NORMAL);
	file->Append(exit);

	m_menu->Append(file, wxT("&File"));

	edit = new wxMenu();

	wxMenuItem* undo;
	undo = new wxMenuItem(edit, wxID_UNDO, wxString(wxT("&Undo")), wxEmptyString, wxITEM_NORMAL);
	edit->Append(undo);

	wxMenuItem* redo;
	redo = new wxMenuItem(edit, wxID_REDO, wxString(wxT("&Redo")), wxEmptyString, wxITEM_NORMAL);
	edit->Append(redo);

	edit->AppendSeparator();

	wxMenuItem* cut;
	cut = new wxMenuItem(edit, wxID_CUT, wxString(wxT("&Cut")), wxEmptyString, wxITEM_NORMAL);
	edit->Append(cut);

	wxMenuItem* copy;
	copy = new wxMenuItem(edit, wxID_COPY, wxString(wxT("C&opy")), wxEmptyString, wxITEM_NORMAL);
	edit->Append(copy);

	wxMenuItem* paste;
	paste = new wxMenuItem(edit, wxID_PASTE, wxString(wxT("&Paste")), wxEmptyString, wxITEM_NORMAL);
	edit->Append(paste);

	edit->AppendSeparator();

	wxMenuItem* preferences;
	preferences = new wxMenuItem(edit, ID_PREFERENCES, wxString(wxT("Pr&eferences")), wxEmptyString, wxITEM_NORMAL);
	edit->Append(preferences);

	m_menu->Append(edit, wxT("&Edit"));

	view = new wxMenu();
	wxMenuItem* zoom;
	zoom = new wxMenuItem(view, ID_ZOOM, wxString(wxT("&Zoom")) , wxEmptyString, wxITEM_NORMAL);
	view->Append(zoom);

	m_menu->Append(view, wxT("&View"));

	help = new wxMenu();
	wxMenuItem* about;
	about = new wxMenuItem(help, wxID_ABOUT, wxString(wxT("&About")), wxEmptyString, wxITEM_NORMAL);
	help->Append(about);

	m_menu->Append(help, wxT("&Help"));

	this->SetMenuBar(m_menu);

	wxBoxSizer* frameSizer;
	frameSizer = new wxBoxSizer(wxVERTICAL);

	m_toolbarPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	wxBoxSizer* toolbarSizer;
	toolbarSizer = new wxBoxSizer(wxHORIZONTAL);

	//m_back = new wxButton(m_toolbarPanel, wxID_ANY, wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	m_back = new wxButton(m_toolbarPanel, wxID_ANY, wxT("Back"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	m_back->SetBitmap(wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_BUTTON));
	m_back->Enable(false);
	toolbarSizer->Add(m_back, 0, wxALIGN_CENTER, 0);

	//m_forward = new wxButton(m_toolbarPanel, wxID_ANY, wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxNO_BORDER);
	m_forward = new wxButton(m_toolbarPanel, wxID_ANY, wxT("Forward"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	m_forward->SetBitmap(wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_BUTTON));
	m_forward->Enable(false);
	toolbarSizer->Add(m_forward, 0, wxALIGN_CENTER, 0);

	m_url = new wxComboBox(m_toolbarPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0|wxNO_BORDER|wxTE_PROCESS_ENTER);
	toolbarSizer->Add(m_url, 1, wxALIGN_CENTER|wxEXPAND, 0);

	//m_go = new wxButton(m_toolbarPanel, wxID_ANY, wxArtProvider::GetBitmap(wxT("gtk-refresh"), wxART_TOOLBAR), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxNO_BORDER);
	m_go = new wxButton(m_toolbarPanel, wxID_ANY, wxT("Reload"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	m_go->SetBitmap(wxArtProvider::GetBitmap(wxT("gtk-refresh"), wxART_BUTTON));
	toolbarSizer->Add(m_go, 0, wxALIGN_CENTER, 0);


	m_toolbarPanel->SetSizer(toolbarSizer);
	m_toolbarPanel->Layout();
	toolbarSizer->Fit(m_toolbarPanel);
	frameSizer->Add(m_toolbarPanel, 0, wxEXPAND,0);

	m_webView = wxWebView::New(this, wxID_ANY, url);
	frameSizer->Add(m_webView, 1, wxALIGN_CENTER|wxEXPAND,0);


	this->SetSizer(frameSizer);
	this->Layout();

	this->Centre(wxBOTH);
	m_url->SetValue(m_webView->GetCurrentURL());
	UpdateState();

	//Button Events
	m_back		->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(webFrame::OnBack), 	NULL,this);
	m_forward	->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(webFrame::OnForward), NULL,this);
	m_url		->Connect(wxEVT_COMMAND_TEXT_ENTER, 	wxCommandEventHandler(webFrame::OnUrl),		NULL,this);
	m_url		->Connect(wxEVT_TEXT, 					wxCommandEventHandler(webFrame::OnUrlMod),	NULL,this);
	m_go		->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(webFrame::OnGo), 		NULL,this);
	//Webview Events
	Connect(m_webView->GetId(),wxEVT_WEBVIEW_LOADED,	wxWebViewEventHandler(webFrame::OnDocumentLoaded), 		NULL, this);
    Connect(m_webView->GetId(),wxEVT_WEBVIEW_NAVIGATED,	wxWebViewEventHandler(webFrame::OnNavigationComplete), 	NULL, this);
    Connect(m_webView->GetId(),wxEVT_WEBVIEW_LOADED,	wxWebViewEventHandler(webFrame::OnDocumentLoaded), 		NULL, this);
}

webFrame::~webFrame()
{
	// Disconnect Events
	m_back		->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, 	wxCommandEventHandler	(webFrame::OnBack), 	NULL,this);
	m_forward	->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, 	wxCommandEventHandler	(webFrame::OnForward),	NULL,this);
	m_url		->Disconnect(wxEVT_COMMAND_TEXT_ENTER,		wxCommandEventHandler	(webFrame::OnUrl),		NULL,this);
	m_go		->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, 	wxCommandEventHandler	(webFrame::OnGo),		NULL,this);
}

/**
  * Method that retrieves the current state from the web control and updates the GUI
  * the reflect this current state.
  */
void webFrame::UpdateState()
{
    m_back		->Enable(m_webView->CanGoBack());
    m_forward	->Enable(m_webView->CanGoForward());

    if (m_webView->IsBusy())
    {
        m_go->SetBitmap(wxArtProvider::GetBitmap("gtk-stop", wxART_BUTTON));
        m_go->SetLabel(wxT("Stop"));
    }
    else
    {
        m_go->SetBitmap(wxArtProvider::GetBitmap("gtk-refresh", wxART_BUTTON));
        m_go->SetLabel(wxT("Reload"));
    }
    if(m_webView->GetCurrentTitle() != wxEmptyString)
    {
		SetTitle(m_webView->GetCurrentTitle() + " - wxWebDemo");
	}
	else
	{
		SetTitle(m_webView->GetCurrentURL() + " - wxWebDemo");
	}
}

/**
  * Callback invoked when user entered an URL and pressed enter
  */
void webFrame::OnUrl(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->LoadURL(procUrl(m_url->GetValue()));
    m_webView->SetFocus();
    UpdateState();
}

/**
    * Callback invoked when user pressed the "back" button
    */
void webFrame::OnBack(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->GoBack();
    UpdateState();
}

/**
  * Callback invoked when user pressed the "forward" button
  */
void webFrame::OnForward(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->GoForward();
    UpdateState();
}

/**
  * Callback invoked when user pressed the "reload/stop" button
  */
void webFrame::OnGo(wxCommandEvent& WXUNUSED(evt))
{
    if (m_webView->IsBusy())
    {
		m_webView->Stop();
	}
	else
	{
		if(m_webView->GetCurrentURL().IsSameAs(m_url->GetValue(),true))
		{
			m_webView->Reload();
		}
		else
		{
			m_webView->LoadURL(m_url->GetValue());
		}
		m_webView->SetFocus();
	}
    UpdateState();
}

void webFrame::OnUrlMod(wxCommandEvent& WXUNUSED(evt))
{
	if(!m_webView->IsBusy())
	{
		if(m_webView->GetCurrentURL().IsSameAs(m_url->GetValue(),true))
		{
			m_go->SetBitmap(wxArtProvider::GetBitmap("gtk-refresh", wxART_BUTTON));
			m_go->SetLabel(wxT("Reload"));
		}
		else
		{
			m_go->SetBitmap(wxArtProvider::GetBitmap("gtk-refresh", wxART_BUTTON));
			m_go->SetLabel(wxT("Go"));
		}
	}
}

/**
  * Callback invoked when user pressed the "reload" button
  */
/*
void webFrame::OnReload(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->Reload();
    UpdateState();
}

void webFrame::OnClearHistory(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->ClearHistory();
    UpdateState();
}

void webFrame::OnEnableHistory(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->EnableHistory(m_tools_enable_history->IsChecked());
    UpdateState();
}
*/

void webFrame::OnUndo(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->Undo();
}

void webFrame::OnRedo(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->Redo();
}
void webFrame::OnCut(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->Cut();
}
void webFrame::OnCopy(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->Copy();
}
void webFrame::OnPaste(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->Paste();
}
/*
void webFrame::OnMode(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->SetEditable(m_edit_mode->IsChecked());
}

/*void webFrame::OnLoadScheme(wxCommandEvent& WXUNUSED(evt))
{
    wxFileName helpfile("../help/doc.zip");
    helpfile.MakeAbsolute();
    wxString path = helpfile.GetFullPath();
    //Under MSW we need to flip the slashes
    path.Replace("\\", "/");
    path = "wxfs:///" + path + ";protocol=zip/doc.htm";
    m_webView->LoadURL(path);
}

void webFrame::OnFind(wxCommandEvent& WXUNUSED(evt))
{
    wxString value = m_webView->GetSelectedText();
    if(value.Len() > 150)
    {
        value.Truncate(150);
    }
    m_find_ctrl->SetValue(value);
    if(!m_find_toolbar->IsShown()){
        m_find_toolbar->Show(true);
        SendSizeEvent();
    }
    m_find_ctrl->SelectAll();
}

void webFrame::OnFindDone(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->Find("");
    m_find_toolbar->Show(false);
    SendSizeEvent();
}

void webFrame::OnFindText(wxCommandEvent& evt)
{
    int flags = 0;

    if(m_find_toolbar_wrap->IsChecked())
        flags |= wxWEBVIEW_FIND_WRAP;
    if(m_find_toolbar_wholeword->IsChecked())
        flags |= wxWEBVIEW_FIND_ENTIRE_WORD;
    if(m_find_toolbar_matchcase->IsChecked())
        flags |= wxWEBVIEW_FIND_MATCH_CASE;
    if(m_find_toolbar_highlight->IsChecked())
        flags |= wxWEBVIEW_FIND_HIGHLIGHT_RESULT;

    if(m_find_toolbar_previous->GetId() == evt.GetId())
        flags |= wxWEBVIEW_FIND_BACKWARDS;

    wxString find_text = m_find_ctrl->GetValue();
    long count = m_webView->Find(find_text, flags);

    if(m_findText != find_text)
    {
        m_findCount = count;
        m_findText = find_text;
    }

    if(count != wxNOT_FOUND || find_text.IsEmpty())
    {
        m_find_ctrl->SetBackgroundColour(*wxWHITE);
    }
    else
    {
        m_find_ctrl->SetBackgroundColour(wxColour(255, 101, 101));
    }

    m_find_ctrl->Refresh();

    //Log the result, note that count is zero indexed.
    if(count != m_findCount)
    {
        count++;
    }
    wxLogMessage("Searching for:%s  current match:%i/%i", m_findText.c_str(), count, m_findCount);
}

/**
  * Callback invoked when there is a request to load a new page (for instance
  * when the user clicks a link)
  */
/*
void webFrame::OnNavigationRequest(wxWebViewEvent& evt)
{
    if(m_info->IsShown())
    {
        m_info->Dismiss();
    }

    wxLogMessage("%s", "Navigation request to '" + evt.GetURL() + "' (target='" +
    evt.GetTarget() + "')");

    wxASSERT(m_webView->IsBusy());

    //If we don't want to handle navigation then veto the event and navigation
    //will not take place, we also need to stop the loading animation
    if(!m_tools_handle_navigation->IsChecked())
    {
        evt.Veto();
        m_toolbar->EnableTool( m_toolbar_stop->GetId(), false );
    }
    else
    {
        UpdateState();
    }
}

/**
  * Callback invoked when a navigation request was accepted
  */
void webFrame::OnNavigationComplete(wxWebViewEvent& evt)
{
    //wxLogMessage("%s", "Navigation complete; url='" + evt.GetURL() + "'");
    m_url->SetValue(m_webView->GetCurrentURL());
    UpdateState();
}

/**
  * Callback invoked when a page is finished loading
  */
void webFrame::OnDocumentLoaded(wxWebViewEvent& evt)
{
    //Only notify if the document is the main frame, not a subframe
    /*if(evt.GetURL() == m_webView->GetCurrentURL())
    {
        //wxLogMessage("%s", "Document loaded; url='" + evt.GetURL() + "'");
    }*/
    m_url->SetValue(m_webView->GetCurrentURL());
    UpdateState();
}
/**
  * On new window, we veto to stop extra windows appearing
  */
/*void webFrame::OnNewWindow(wxWebViewEvent& evt)
{
    wxLogMessage("%s", "New window; url='" + evt.GetURL() + "'");

    //If we handle new window events then just load them in this window as we
    //are a single window browser
    if(m_tools_handle_new_window->IsChecked())
        m_webView->LoadURL(evt.GetURL());

    UpdateState();
}*/

void webFrame::OnTitleChanged(wxWebViewEvent& evt)
{
    SetTitle(evt.GetString());
    wxLogMessage("%s", "Title changed; title='" + evt.GetString() + "'");
}

/**
  * Invoked when user selects the "View Source" menu item
  */
/*void webFrame::OnViewSourceRequest(wxCommandEvent& WXUNUSED(evt))
{
    SourceViewDialog dlg(this, m_webView->GetPageSource());
    dlg.ShowModal();
}*/

/**
  * Invoked when user selects the "Menu" item
  */
/*void webFrame::OnToolsClicked(wxCommandEvent& WXUNUSED(evt))
{
    if(m_webView->GetCurrentURL() == "")
        return;

    m_tools_tiny->Check(false);
    m_tools_small->Check(false);
    m_tools_medium->Check(false);
    m_tools_large->Check(false);
    m_tools_largest->Check(false);

    wxWebViewZoom zoom = m_webView->GetZoom();
    switch (zoom)
    {
    case wxWEBVIEW_ZOOM_TINY:
        m_tools_tiny->Check();
        break;
    case wxWEBVIEW_ZOOM_SMALL:
        m_tools_small->Check();
        break;
    case wxWEBVIEW_ZOOM_MEDIUM:
        m_tools_medium->Check();
        break;
    case wxWEBVIEW_ZOOM_LARGE:
        m_tools_large->Check();
        break;
    case wxWEBVIEW_ZOOM_LARGEST:
        m_tools_largest->Check();
        break;
    }

    m_edit_cut->Enable(m_webView->CanCut());
    m_edit_copy->Enable(m_webView->CanCopy());
    m_edit_paste->Enable(m_webView->CanPaste());

    m_edit_undo->Enable(m_webView->CanUndo());
    m_edit_redo->Enable(m_webView->CanRedo());

    m_selection_clear->Enable(m_webView->HasSelection());
    m_selection_delete->Enable(m_webView->HasSelection());

    m_context_menu->Check(m_webView->IsContextMenuEnabled());

    //Firstly we clear the existing menu items, then we add the current ones
    wxMenuHistoryMap::const_iterator it;
    for( it = m_histMenuItems.begin(); it != m_histMenuItems.end(); ++it )
    {
        m_tools_history_menu->Destroy(it->first);
    }
    m_histMenuItems.clear();

    wxVector<wxSharedPtr<wxWebViewHistoryItem> > back = m_webView->GetBackwardHistory();
    wxVector<wxSharedPtr<wxWebViewHistoryItem> > forward = m_webView->GetForwardHistory();

    wxMenuItem* item;

    unsigned int i;
    for(i = 0; i < back.size(); i++)
    {
        item = m_tools_history_menu->AppendRadioItem(wxID_ANY, back[i]->GetTitle());
        m_histMenuItems[item->GetId()] = back[i];
        Connect(item->GetId(), wxEVT_MENU,
                wxCommandEventHandler(webFrame::OnHistory), NULL, this );
    }

    wxString title = m_webView->GetCurrentTitle();
    if ( title.empty() )
        title = "(untitled)";
    item = m_tools_history_menu->AppendRadioItem(wxID_ANY, title);
    item->Check();

    //No need to connect the current item
    m_histMenuItems[item->GetId()] = wxSharedPtr<wxWebViewHistoryItem>(new wxWebViewHistoryItem(m_webView->GetCurrentURL(), m_webView->GetCurrentTitle()));

    for(i = 0; i < forward.size(); i++)
    {
        item = m_tools_history_menu->AppendRadioItem(wxID_ANY, forward[i]->GetTitle());
        m_histMenuItems[item->GetId()] = forward[i];
        Connect(item->GetId(), wxEVT_TOOL,
                wxCommandEventHandler(webFrame::OnHistory), NULL, this );
    }

    wxPoint position = ScreenToClient( wxGetMousePosition() );
    PopupMenu(m_tools_menu, position.x, position.y);
}

/**
  * Invoked when user selects the zoom size in the menu
  */
/*void webFrame::OnSetZoom(wxCommandEvent& evt)
{
    if (evt.GetId() == m_tools_tiny->GetId())
    {
        m_webView->SetZoom(wxWEBVIEW_ZOOM_TINY);
    }
    else if (evt.GetId() == m_tools_small->GetId())
    {
        m_webView->SetZoom(wxWEBVIEW_ZOOM_SMALL);
    }
    else if (evt.GetId() == m_tools_medium->GetId())
    {
        m_webView->SetZoom(wxWEBVIEW_ZOOM_MEDIUM);
    }
    else if (evt.GetId() == m_tools_large->GetId())
    {
        m_webView->SetZoom(wxWEBVIEW_ZOOM_LARGE);
    }
    else if (evt.GetId() == m_tools_largest->GetId())
    {
        m_webView->SetZoom(wxWEBVIEW_ZOOM_LARGEST);
    }
    else
    {
        wxFAIL;
    }
}

void webFrame::OnZoomLayout(wxCommandEvent& WXUNUSED(evt))
{
    if(m_tools_layout->IsChecked())
        m_webView->SetZoomType(wxWEBVIEW_ZOOM_TYPE_LAYOUT);
    else
        m_webView->SetZoomType(wxWEBVIEW_ZOOM_TYPE_TEXT);
}

/*void webFrame::OnHistory(wxCommandEvent& evt)
{
    m_webView->LoadHistoryItem(m_histMenuItems[evt.GetId()]);
}
*/
/*void webFrame::OnRunScript(wxCommandEvent& WXUNUSED(evt))
{
    wxTextEntryDialog dialog(this, "Enter JavaScript to run.", wxGetTextFromUserPromptStr, "", wxOK|wxCANCEL|wxCENTRE|wxTE_MULTILINE);
    if(dialog.ShowModal() == wxID_OK)
    {
        m_webView->RunScript(dialog.GetValue());
    }
}*/

void webFrame::OnClearSelection(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->ClearSelection();
}

void webFrame::OnDeleteSelection(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->DeleteSelection();
}

void webFrame::OnSelectAll(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->SelectAll();
}

/**
  * Callback invoked when a loading error occurs
  */
void webFrame::OnError(wxWebViewEvent& evt)
{
#define WX_ERROR_CASE(type) \
    case type: \
        category = #type; \
        break;

    wxString category;
    switch (evt.GetInt())
    {
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_CONNECTION);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_CERTIFICATE);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_AUTH);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_SECURITY);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_NOT_FOUND);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_REQUEST);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_USER_CANCELLED);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_OTHER);
    }

    wxLogMessage("%s", "Error; url='" + evt.GetURL() + "', error='" + category + " (" + evt.GetString() + ")'");

    //Show the info bar with an error
    /*m_info->ShowMessage(_("An error occurred loading ") + evt.GetURL() + "\n" +
    "'" + category + "'", wxICON_ERROR);*/

    UpdateState();
}

/**
  * Invoked when user selects "Print" from the menu
  */
void webFrame::OnPrint(wxCommandEvent& WXUNUSED(evt))
{
    m_webView->Print();
}
/*
SourceViewDialog::SourceViewDialog(wxWindow* parent, wxString source) :
                  wxDialog(parent, wxID_ANY, "Source Code",
                           wxDefaultPosition, wxSize(700,500),
                           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxStyledTextCtrl* text = new wxStyledTextCtrl(this, wxID_ANY);
    text->SetMarginWidth(1, 30);
    text->SetMarginType(1, wxSTC_MARGIN_NUMBER);
    text->SetText(source);

    text->StyleClearAll();
    text->SetLexer(wxSTC_LEX_HTML);
    text->StyleSetForeground(wxSTC_H_DOUBLESTRING, wxColour(255,0,0));
    text->StyleSetForeground(wxSTC_H_SINGLESTRING, wxColour(255,0,0));
    text->StyleSetForeground(wxSTC_H_ENTITY, wxColour(255,0,0));
    text->StyleSetForeground(wxSTC_H_TAG, wxColour(0,150,0));
    text->StyleSetForeground(wxSTC_H_TAGUNKNOWN, wxColour(0,150,0));
    text->StyleSetForeground(wxSTC_H_ATTRIBUTE, wxColour(0,0,150));
    text->StyleSetForeground(wxSTC_H_ATTRIBUTEUNKNOWN, wxColour(0,0,150));
    text->StyleSetForeground(wxSTC_H_COMMENT, wxColour(150,150,150));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(text, 1, wxEXPAND);
    SetSizer(sizer);
}
*/
