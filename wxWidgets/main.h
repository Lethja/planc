///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  3 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __MAIN_H__
#define __MAIN_H__

#include "config.h"

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/tglbtn.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/webview.h>
#include <wx/webviewarchivehandler.h>
#include <wx/webviewfshandler.h>
#include <wx/treectrl.h>
#include <wx/splitter.h>

///////////////////////////////////////////////////////////////////////////

#define ID_NEW 1000
#define ID_EXIT 1001
#define ID_CUT 1002
#define ID_COPY 1003
#define ID_PASTE 1004
#define ID_PREFERENCES 1005
#define ID_ZOOM 1006
#define ID_ABOUT 1007

class WebApp : public wxApp
{
public:
    WebApp() :
        m_url(wxEmptyString)
    {
    }

    virtual bool OnInit();

#if wxUSE_CMDLINE_PARSER
    virtual void OnInitCmdLine(wxCmdLineParser& parser)
    {
        wxApp::OnInitCmdLine(parser);

        parser.AddParam("URL to open",
                        wxCMD_LINE_VAL_STRING,
                        wxCMD_LINE_PARAM_OPTIONAL);
    }

    virtual bool OnCmdLineParsed(wxCmdLineParser& parser)
    {
        if ( !wxApp::OnCmdLineParsed(parser) )
            return false;

        if ( parser.GetParamCount() )
            m_url = parser.GetParam(0);

        return true;
    }
#endif // wxUSE_CMDLINE_PARSER

private:
    wxString m_url;
};

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
//FIX ME! Need to get file parameters
    { wxCMD_LINE_SWITCH, "h", "help", "show this help message",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP},
    {wxCMD_LINE_PARAM,NULL,NULL,"[URL]",wxCMD_LINE_VAL_STRING,
        wxCMD_LINE_PARAM_MULTIPLE|wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_NONE}
};

///////////////////////////////////////////////////////////////////////////////
/// Class webFrame
///////////////////////////////////////////////////////////////////////////////

class webFrame : public wxFrame 
{
	private:
		wxString p_url;
	protected:
		wxMenuBar* m_menu;
		wxMenu* file;
		wxMenu* edit;
		wxMenu* view;
		wxMenu* help;
		wxPanel* m_toolbarPanel;
		wxPanel* m_footer;
		wxButton* m_back;
		wxButton* m_forward;
		wxComboBox* m_url;
		wxButton* m_go;
		wxWebView* m_webView;
		wxBitmapToggleButton * m_side;
		
		//Events from webview
		void UpdateState();
		void OnIdle(wxIdleEvent& evt);
		void OnUrl(wxCommandEvent& evt);
		void OnUrlMod(wxCommandEvent& evt);
		void OnBack(wxCommandEvent& evt);
		void OnForward(wxCommandEvent& evt);
		void OnGo(wxCommandEvent& evt);
		void OnClearHistory(wxCommandEvent& evt);
		void OnEnableHistory(wxCommandEvent& evt);
		void OnNavigationRequest(wxWebViewEvent& evt);
		void OnNavigationComplete(wxWebViewEvent& evt);
		void OnDocumentLoaded(wxWebViewEvent& evt);
		void OnNewWindow(wxWebViewEvent& evt);
		void OnTitleChanged(wxWebViewEvent& evt);
		void OnViewSourceRequest(wxCommandEvent& evt);
		void OnToolsClicked(wxCommandEvent& evt);
		void OnSetZoom(wxCommandEvent& evt);
		void OnError(wxWebViewEvent& evt);
		void OnPrint(wxCommandEvent& evt);
		void OnCut(wxCommandEvent& evt);
		void OnCopy(wxCommandEvent& evt);
		void OnPaste(wxCommandEvent& evt);
		void OnUndo(wxCommandEvent& evt);
		void OnRedo(wxCommandEvent& evt);
		void OnMode(wxCommandEvent& evt);
		void OnZoomLayout(wxCommandEvent& evt);
		void OnHistory(wxCommandEvent& evt);
		void OnRunScript(wxCommandEvent& evt);
		void OnClearSelection(wxCommandEvent& evt);
		void OnDeleteSelection(wxCommandEvent& evt);
		void OnSelectAll(wxCommandEvent& evt);
		void OnLoadScheme(wxCommandEvent& evt);
		void OnFind(wxCommandEvent& evt);
		void OnFindDone(wxCommandEvent& evt);
		void OnFindText(wxCommandEvent& evt);
		void OnFindOptions(wxCommandEvent& evt);
		void OnEnableContextMenu(wxCommandEvent& evt);
	public:
		webFrame(const wxString& url);
		virtual ~webFrame();
};

#endif //__MAIN_H__
