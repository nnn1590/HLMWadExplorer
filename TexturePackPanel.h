#ifndef __TexturePackPanel__
#define __TexturePackPanel__

/**
@file
Subclass of BaseTexturePackPanel, which is generated by wxFormBuilder.
*/

#include "HLMWADFrames.h"

//// end generated include

#include "TexturePack.h"

/** Implementing BaseTexturePackPanel */
class TexturePackPanel : public BaseTexturePackPanel
{
	protected:
		// Handlers for BaseTexturePackPanel events.
		void OnTextureListBoxSelected( wxCommandEvent& event );
		void OnFrameSpinCtrlChanged( wxSpinEvent& event );
		void OnFrameSpinCtrlEnterPressed( wxCommandEvent& event );
		void OnColourPickerChanged(wxColourPickerEvent& event);
		void OnZoomSpinCtrlChanged(wxSpinEvent& event);
		void OnExportGIFClicked(wxCommandEvent& event);
public:
		/** Constructor */
		TexturePackPanel( wxWindow* parent );
	//// end generated class members
	
		void LoadTexture(wxInputStream& iStr, wxBitmap bitmap);

	private:
		wxBitmap m_bitmap;
		wxSharedPtr<TexturePack> m_texturePack;

		void UpdateFrameImage();

		wxBitmap GetFrameBitmap(const Frame& frame, int zoom = 1);
};

#endif // __TexturePackPanel__