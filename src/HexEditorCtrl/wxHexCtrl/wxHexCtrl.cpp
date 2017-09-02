/***********************************(GPL)********************************
*   wxHexEditor is a hex edit tool for editing massive files in Linux   *
*   Copyright (C) 2010  Erdem U. Altinyurt                              *
*                                                                       *
*   This program is free software; you can redistribute it and/or       *
*   modify it under the terms of the GNU General Public License         *
*   as published by the Free Software Foundation; either version 2      *
*   of the License.                                                     *
*                                                                       *
*   This program is distributed in the hope that it will be useful,     *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of      *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
*   GNU General Public License for more details.                        *
*                                                                       *
*   You should have received a copy of the GNU General Public License   *
*   along with this program;                                            *
*   if not, write to the Free Software Foundation, Inc.,                *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA        *
*                                                                       *
*               home  : www.wxhexeditor.org                             *
*               email : spamjunkeater@gmail.com                         *
*************************************************************************/


#include "wxHexCtrl.h"
#include <wx/encconv.h>
#include <wx/fontmap.h>

BEGIN_EVENT_TABLE(wxHexCtrl,wxScrolledWindow )
	EVT_CHAR( wxHexCtrl::OnChar )
	EVT_SIZE( wxHexCtrl::OnSize )
	EVT_PAINT( wxHexCtrl::OnPaint )
	EVT_LEFT_DOWN( wxHexCtrl::OnMouseLeft )
	//EVT_LEFT_DOWN( wxHexOffsetCtrl::OnMouseLeft )
	//EVT_MOUSE( wxHexCtrl::OnResize)
	EVT_RIGHT_DOWN( wxHexCtrl::OnMouseRight )
	EVT_MENU( __idTagAddSelect__, wxHexCtrl::OnTagAddSelection )
	EVT_MENU( __idTagEdit__, wxHexCtrl::OnTagEdit )
	EVT_MOTION( wxHexCtrl::OnMouseMove )
	EVT_SET_FOCUS( wxHexCtrl::OnFocus )
	//EVT_KILL_FOCUS( wxHexCtrl::OnKillFocus ) //Not needed
END_EVENT_TABLE()


//#define _Use_Alternate_DrawText_ //For debugged drawtext for wx 2.9.x on Mac

//IMPLEMENT_DYNAMIC_CLASS(wxHexCtrl, wxScrolledWindow)

wxHexCtrl::wxHexCtrl(wxWindow *parent,
			wxWindowID id,
			const wxString &value,
			const wxPoint &pos,
			const wxSize &size,
			long style,
			const wxValidator& validator)
			: wxScrolledWindow( parent, id,
								pos, size,
								wxSUNKEN_BORDER )
	{
	HexDefaultAttr = wxTextAttr(
								wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ),
								wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ),
								wxFont(
									10,
									wxFONTFAMILY_MODERN,	// family
									wxFONTSTYLE_NORMAL,	// style
									wxFONTWEIGHT_BOLD,// weight
									true,				// underline
									wxT(""),			// facename
									wxFONTENCODING_CP437) );// msdos encoding

	//Need to create object before Draw operation.
	ZebraStriping=new int;
	*ZebraStriping=-1;

	CtrlType=0;

	DrawCharByChar=false;

	internalBufferDC=NULL;
	internalBufferBMP=NULL;

	HexFormat = wxT("xx ");

	mycaret=NULL;
	SetSelectionStyle( HexDefaultAttr );

	HexDefaultAttr = wxTextAttr(
								//*wxBLACK, //Deprecated :p
								wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ),
								wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ),
								//*wxWHITE, //Deprecated :p
								//wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ),
								wxFont(
									10,
									wxFONTFAMILY_MODERN,	// family
									wxFONTSTYLE_NORMAL,	// style
									wxFONTWEIGHT_NORMAL,// weight
									false,				// underline
									wxT(""),			// facename
									wxFONTENCODING_CP437) );// msdos encoding

   ClearSelection( false );
   SetDefaultStyle( HexDefaultAttr );

   m_Caret.x = m_Caret.y =
   m_Window.x = m_Window.y = 1;
   m_Margin.x = m_Margin.y = 0;
	LastRightClickPosition = wxPoint(0,0);
   select.selected = false;

	CreateCaret();

  //  ChangeSize();

   //wxCaret *caret = GetCaret();
   if ( mycaret )
		mycaret->Show(false);

}
wxHexCtrl::~wxHexCtrl()
{
   Clear();
   //m_text.Clear();
   wxCaretSuspend cs(this);
   /*
   wxBufferedPaintDC dc( this );
   PrepareDC( dc );
   dc.SetFont( HexDefaultAttr.GetFont() );
	dc.SetTextForeground( HexDefaultAttr.GetTextColour() );
   dc.SetTextBackground( HexDefaultAttr.GetBackgroundColour() );
	wxBrush bbrush( HexDefaultAttr.GetBackgroundColour() );
   dc.SetBackground(bbrush );
   dc.Clear();
   */
}

void wxHexCtrl::Clear( bool RePaint, bool cursor_reset ){
	m_text.Clear();
	if( cursor_reset )
		SetInsertionPoint(0);
	OnTagHideAll();
	ClearSelection( RePaint );
	WX_CLEAR_ARRAY(TagArray);
	}

void wxHexCtrl::CreateCaret(){
   wxCaret *caret = new wxCaret(this, m_CharSize.x, m_CharSize.y);
   mycaret = caret;
#ifdef _DEBUG_CARET_
	std::cout << "Caret = 0x"<< (intptr_t) caret <<  " - mycaret= 0x" << (intptr_t) mycaret << "m_charSize.x" << m_CharSize.x << std::endl;
#endif
   SetCaret(caret);
   caret->Move(m_Margin.x, m_Margin.x);
   caret->Show();
	}

void wxHexCtrl::FirstLine( bool MoveCaret ){
	m_Caret.y = 0;
	if(MoveCaret)
		DoMoveCaret();
	}

void wxHexCtrl::LastLine( bool MoveCaret ){
	m_Caret.y = m_Window.y - 1;
	if ( IsDenied() )
		NextChar( false );
	if ( GetInsertionPoint() > GetLastPosition() ){
		wxBell();
		SetInsertionPoint( GetLastPosition() );
		}
	if(MoveCaret)
		DoMoveCaret();
	}

void wxHexCtrl::Home( bool MoveCaret ){
	m_Caret.x = 0;
	if( MoveCaret )
		DoMoveCaret();
	}

void wxHexCtrl::End( bool MoveCaret ){
	m_Caret.x = m_Window.x - 1;
	if ( IsDenied() )
		PrevChar( false );
	if ( GetInsertionPoint() > GetLastPosition() ){
		wxBell();
		SetInsertionPoint( GetLastPosition() );
		}
	if(MoveCaret)
		DoMoveCaret();
	}

void wxHexCtrl::PrevChar( bool MoveCaret ){
	if ( !m_Caret.x-- ){
		End( false );
		PrevLine( false );
		}
	if ( IsDenied() )
		PrevChar( false );
	if( MoveCaret )
		DoMoveCaret();
	}

void wxHexCtrl::NextChar( bool MoveCaret ) {
	if ( ++m_Caret.x == m_Window.x ){
		Home( false );
		NextLine( false );
		}
	else if ( IsDenied() )
		NextChar( false );
	else if ( GetInsertionPoint() > GetLastPosition() ){
		wxBell();
		SetInsertionPoint( GetLastPosition() );
		}
	if( MoveCaret )
		DoMoveCaret();
	}

void wxHexCtrl::PrevLine( bool MoveCaret ){
	if ( !m_Caret.y-- ){
		m_Caret.y++;
		Home( false );
		wxBell();
		}
	if( MoveCaret )
		DoMoveCaret();
	}

void wxHexCtrl::NextLine( bool MoveCaret ){
	if ( ++m_Caret.y == m_Window.y ) {
		m_Caret.y--;
		End( false );
		wxBell();
		}
	else if ( GetInsertionPoint() > GetLastPosition() ){
		wxBell();
		SetInsertionPoint( GetLastPosition() );
		}
	if( MoveCaret )
		DoMoveCaret();
	}

inline bool wxHexCtrl::IsDenied( int x ){	// State Of The Art :) Hex plotter function by idents avoiding some X axes :)
	return IsDeniedCache[x];
	}

inline bool wxHexCtrl::IsDenied_NoCache( int x ){	// State Of The Art :) Hex plotter function by idents avoiding some X axes :)
//		x%=m_Window.x;						// Discarding y axis noise
	if(1){ //EXPERIMENTAL
		if( ( ( m_Window.x - 1 ) % HexFormat.Len() == 0 )	// For avoid hex divorcings
			&& ( x == m_Window.x - 1 ))
			return true;
		return HexFormat[x%(HexFormat.Len())]==' ';
		}

	if( ( ( m_Window.x - 1 ) % 3 == 0 )		// For avoid hex divorcings
		&& ( x == m_Window.x - 1 ))
		return true;
//	if( x == 3*8 )
//		return true;
	return !( ( x + 1 ) % 3 );				// Byte coupling
	}

int wxHexCtrl::xCountDenied( int x ){		//Counts denied character locations (spaces) on given x coordination
	for( int i = 0, denied = 0 ; i <  m_Window.x ; i++ ){
		if( IsDenied(i) )
			denied++;
		if( i == x )
			return denied;
		}
	return -1;
	}

int wxHexCtrl::CharacterPerLine( bool NoCache ){	//Without spaces
	if( !NoCache )
		return CPL;
	int avoid=0;
	for ( int x = 0 ; x < m_Window.x ; x++)
		avoid += IsDeniedCache[x];
	CPL=m_Window.x - avoid;
	//std::cout << "CPL: " << CPL << std::endl;
	return ( m_Window.x - avoid );
	}

int wxHexCtrl::GetInsertionPoint( void ){
	return ( m_Caret.x - xCountDenied(m_Caret.x) ) + CharacterPerLine() * m_Caret.y;
	}

void wxHexCtrl::SetInsertionPoint( unsigned int pos ){
	if(pos > m_text.Length())
		pos = m_text.Length();
	pos = ToVisiblePosition(pos);
	MoveCaret( wxPoint(pos%m_Window.x , pos/m_Window.x) );
	}

int wxHexCtrl::ToVisiblePosition( int InternalPosition ){	// I mean for this string on hex editor  "00 FC 05 C[C]" , while [] is cursor
	if( CharacterPerLine() == 0 ) return 0;					// Visible position is 8 but internal position is 11
	int y = InternalPosition / CharacterPerLine();
	int x = InternalPosition - y * CharacterPerLine();
	for( int i = 0, denied = 0 ; i < m_Window.x ; i++ ){
		if( IsDenied(i) ) denied++;
		if( i - denied == x )
			return ( i + y * m_Window.x );
		}
	//wxLogError(wxString::Format(_T("Fatal error at fx ToVisiblePosition(%d)"),InternalPosition));
	return 0;
	}

int wxHexCtrl::ToInternalPosition( int VisiblePosition ){
	int y = VisiblePosition / m_Window.x;
	int x = VisiblePosition - y * m_Window.x;
	return ( x - xCountDenied(x) + y * CharacterPerLine() );
	}
																	// 00 15 21 CC FC
																	// 55 10 49 54 [7]7
wxPoint wxHexCtrl::InternalPositionToVisibleCoord( int position ){	// Visible position is 19, Visible Coord is (9,2)
	if( position < 0 )
		wxLogError(wxString::Format(_T("Fatal error at fx InternalPositionToVisibleCoord(%d)"),position));
	int x = m_Window.x? m_Window.x : 1;	//prevents divide zero error;
	int pos = ToVisiblePosition( position );
	return wxPoint( pos - (pos / x) * x, pos / x );
	}

int wxHexCtrl::PixelCoordToInternalPosition( wxPoint mouse ){
	mouse = PixelCoordToInternalCoord( mouse );
	return ( mouse.x - xCountDenied(mouse.x) + mouse.y * CharacterPerLine() );
	}

wxPoint wxHexCtrl::PixelCoordToInternalCoord( wxPoint mouse ){
	mouse.x = ( mouse.x < 0 ? 0 : mouse.x);
	mouse.x = ( mouse.x > m_CharSize.x*m_Window.x ? m_CharSize.x*m_Window.x-1 : mouse.x);
	mouse.y = ( mouse.y < 0 ? 0 : mouse.y);
	mouse.y = ( mouse.y > m_CharSize.y*m_Window.y ? m_CharSize.y*m_Window.y-1 : mouse.y);
	int x = (mouse.x - m_Margin.x) / m_CharSize.x;
	int y = (mouse.y - m_Margin.y) / m_CharSize.y;
	return wxPoint(x,y);
	}

void wxHexCtrl::SetFormat( wxString fmt ){
	HexFormat = fmt;
//	for(int i=0 ; i < m_Window.x+1 ; i++)
//		IsDeniedCache[i]=IsDenied_NoCache(i);
	}

wxString wxHexCtrl::GetFormat( void ){
	return HexFormat;
	}

void wxHexCtrl::SetDefaultStyle( wxTextAttr& new_attr ){
	HexDefaultAttr = new_attr;

   wxClientDC dc(this);
   dc.SetFont( HexDefaultAttr.GetFont() );
   SetFont( HexDefaultAttr.GetFont() );
   m_CharSize.y = dc.GetCharHeight();
   m_CharSize.x = dc.GetCharWidth();

   wxCaret *caret = GetCaret();
#ifdef _DEBUG_CARET_
	std::cout << "Caret = 0x"<< (intptr_t) caret <<  " - mycaret= 0x" << (intptr_t) mycaret << "m_charSize.x" << m_CharSize.x << std::endl;
#endif
    if ( caret )
		caret->SetSize(m_CharSize.x, m_CharSize.y);
	RePaint();
}

void wxHexCtrl::SetSelectionStyle( wxTextAttr& new_attr ){
	wxColourData clrData;
	clrData.SetColour( new_attr.GetTextColour() );
	select.FontClrData = clrData;
	clrData.SetColour( new_attr.GetBackgroundColour() );
	select.NoteClrData = clrData;
	}

void wxHexCtrl::SetSelection( unsigned start, unsigned end ){
	select.start = start;
	select.end = end;
	select.selected = true;
	RePaint();
	}

void wxHexCtrl::ClearSelection( bool repaint ){
	select.start = 0;
	select.end = 0;
	select.selected = false;
	if( repaint )
		RePaint();
	}

void wxHexCtrl::MoveCaret(wxPoint p){
#ifdef _DEBUG_CARET_
	std::cout << "MoveCaret(wxPoint) Coordinate X:Y = " << p.x	<< " " << p.y << std::endl;
#endif
   m_Caret = p;
   DoMoveCaret();
}

void wxHexCtrl::MoveCaret(int x){
#ifdef _DEBUG_CARET_
	std::cout << "MoveCaret(ınt) = " << x << std::endl;
#endif
	m_Caret.y = x/CharacterPerLine();
   m_Caret.x = x - m_Caret.y*CharacterPerLine();
   DoMoveCaret();
}

void wxHexCtrl::DoMoveCaret(){
   wxCaret *caret = GetCaret();
#ifdef _DEBUG_CARET_
	std::cout << "Caret = 0x"<< (intptr_t) caret <<  " - mycaret= 0x" << (intptr_t) mycaret << "m_charSize.x" << m_CharSize.x << std::endl;
#endif
   if ( caret )
		caret->Move(m_Margin.x + m_Caret.x * m_CharSize.x,
                    m_Margin.x + m_Caret.y * m_CharSize.y);
}

inline wxMemoryDC* wxHexCtrl::CreateDC(){
//	wxBitmap *bmp=new wxBitmap(this->GetSize().GetWidth(), this->GetSize().GetHeight());
//	wxMemoryDC *dcTemp = new wxMemoryDC();
//	dcTemp->SelectObject(*bmp);
//	return dcTemp;
	if(internalBufferDC != NULL)
		delete internalBufferDC;
	if(internalBufferBMP != NULL)
		delete internalBufferBMP;

	// Note that creating a 0-sized bitmap would fail, so ensure that we create
	// at least 1*1 one.
	wxSize sizeBmp = GetSize();
	sizeBmp.IncTo(wxSize(1, 1));
	internalBufferBMP= new wxBitmap(sizeBmp);
	internalBufferDC = new wxMemoryDC();
	internalBufferDC->SelectObject(*internalBufferBMP);
	return internalBufferDC;
	}

//inline wxDC* wxHexCtrl::UpdateDC(wxDC *dcTemp){
inline wxDC* wxHexCtrl::UpdateDC(wxDC *xdc ){
	wxDC *dcTemp;
	if( xdc )
		dcTemp = xdc;
	else if(internalBufferDC==NULL){
		internalBufferDC = CreateDC();
		dcTemp = internalBufferDC;
		}
	else
		dcTemp = internalBufferDC;
	//wxBufferedPaintDC *dcTemp= new wxBufferedPaintDC(this); //has problems with MacOSX

#ifdef _DEBUG_SIZE_
		std::cout << "wxHexCtrl::Update Sizes: " << this->GetSize().GetWidth() << ":" << this->GetSize().GetHeight() << std::endl;
#endif

	dcTemp->SetFont( HexDefaultAttr.GetFont() );
	dcTemp->SetTextForeground( HexDefaultAttr.GetTextColour() );
	dcTemp->SetTextBackground( HexDefaultAttr.GetBackgroundColour() ); //This will be overriden by Zebra stripping
	wxBrush dbrush( HexDefaultAttr.GetBackgroundColour() );

	dcTemp->SetBackground(dbrush );
	dcTemp->SetBackgroundMode( wxSOLID ); // overwrite old value
	dcTemp->Clear();

	wxString line;
	line.Alloc( m_Window.x+1 );
	wxColour col_standart(HexDefaultAttr.GetBackgroundColour());

	wxColour col_zebra(0x00FFEEEE);
// TODO (death#1#): Remove colour lookup for speed up
	wxString Colour;
	if( wxConfig::Get()->Read( _T("ColourHexBackgroundZebra"), &Colour) )
		col_zebra.Set( Colour );

	size_t textLenghtLimit = 0;
	size_t textLength=m_text.Length();
//	char bux[1000];  //++//

	//Normal process
#define Hex_2_Color_Engine_Prototype 0
	if( !Hex_2_Color_Engine_Prototype || CtrlType==2 ){
		dcTemp->SetPen(*wxTRANSPARENT_PEN);
	   //Drawing line by line
		for ( int y = 0 ; y < m_Window.y; y++ ){	//Draw base hex value without color tags
			line.Empty();

			//Prepare for zebra stripping
			if (*ZebraStriping != -1 ){
				dcTemp->SetTextBackground( (y+*ZebraStriping)%2 ? col_standart : col_zebra);

				//This fills empty regions at Zebra Stripes when printed char with lower than defined
				if(DrawCharByChar){
					dcTemp->SetBrush( wxBrush( (y+*ZebraStriping)%2 ? col_standart : col_zebra ));
					dcTemp->DrawRectangle( m_Margin.x, m_Margin.y + y * m_CharSize.y, m_Window.x*m_CharSize.x, m_CharSize.y);
					}
				}

			for ( int x = 0 ; x < m_Window.x; x++ ){
				if( IsDenied(x)){
					line += wxT(' ');
					//bux[x]=' ';//++//
					continue;
					}
				if(textLenghtLimit >= textLength)
					break;
		//		bux[x]=CharAt(z);//++//
				line += CharAt(textLenghtLimit++);
				//dcTemp->DrawText( wxString::From8BitData(&t), m_Margin.x + x*m_CharSize.x, m_Margin.y + y * m_CharSize.y );
				}

			// For encodings that have variable font with, we need to write characters one by one.
			if(DrawCharByChar)
				for( unsigned q = 0 ; q < line.Len() ;q++ )
					dcTemp->DrawText( line[q], m_Margin.x + q*m_CharSize.x, m_Margin.y + y * m_CharSize.y );
			else
				dcTemp->DrawText( line, m_Margin.x, m_Margin.y + y * m_CharSize.y );
			}
		}
	else{
/*** Hex to Color Engine Prototype ***/
		char chr;
		unsigned char chrP,chrC;
		unsigned char R,G,B;
		wxColour RGB;
		chrP=0;
		int col[256];
		//Prepare 8bit to 32Bit color table for speed
		///Bit    7  6  5  4  3  2  1  0
		///Data   R  R  R  G  G  G  B  B
		///OnWX   B  B  G  G  G  R  R  R

		for(unsigned chrC=0;chrC<256;chrC++){
			R=(chrC>>5)*0xFF/7;
			G=(0x07 & (chrC>>2))*0xFF/7;
			B=(0x03 & chrC)*0xFF/3;
			col[chrC]=B<<16|G<<8|R;
			}

//		//Monochrome Palettes
//		for(int chrC=0;chrC<256;chrC++){
//			col[chrC]=chrC*0x010101; //W
//			//col[chrC]=chrC; 	//R
//			//col[chrC]=chrC<<8; 	//G
//			//col[chrC]=chrC<<16; //B
//			}

		wxString RenderedHexByte;
		for ( int y = 0 ; y < m_Window.y; y++ ){
			for ( int x = 0 ; x < m_Window.x; ){
				if(CtrlType==0){
					RenderedHexByte.Empty();
					/*while( IsDenied( x ) ){
						RenderedHexByte=" ";
						x++;
						}
					*/
					if(textLenghtLimit >= textLength)
						break;

					//First half of byte
					RenderedHexByte += CharAt(textLenghtLimit++);
					chr = RenderedHexByte.ToAscii().operator[](0);
					chrC = atoh( chr ) << 4;

					//Space could be here
					int i=1;
					while( IsDenied( x+i ) ){
						RenderedHexByte+=wxT(" ");
						i++;
						}

					//Second half of byte.
					RenderedHexByte += CharAt(textLenghtLimit++);
					chr = RenderedHexByte.ToAscii().operator[](1);
					chrC |= atoh( chr );
					//chrC = (atoh( RenderedHexByte.ToAscii()[0] ) << 4) | atoh( RenderedHexByte.ToAscii()[1] );

					//Trailing HEX space
					i++;
					while( IsDenied( x+i ) ){
						RenderedHexByte+=wxT(" ");
						i++;
						}

					RGB.Set(col[chrC]);
					//dcTemp->SetTextBackground( wxColour(R,G,B) );
					dcTemp->SetTextBackground( RGB );
					dcTemp->DrawText( RenderedHexByte, m_Margin.x + x*m_CharSize.x, m_Margin.y + y * m_CharSize.y );
					chrC = 0;
					x+=i;
					}
				//Text Coloring
				else{
					//Not accurate since text buffer is changed due encoding translation
					chrC=CharAt(textLenghtLimit);
					wxString stt=CharAt(textLenghtLimit++);
					RGB.Set(col[chrC]);
					dcTemp->SetTextBackground( RGB );
					dcTemp->DrawText( stt, m_Margin.x + x*m_CharSize.x, m_Margin.y + y * m_CharSize.y );
					x++;
					}
				}
			}
	}//Hex_2_Color_Engine_Prototype

#ifndef _Use_Graphics_Contex_ //Uding_Graphics_Context disable TAG painting at buffer.
	int TAC = TagArray.Count();
	if( TAC != 0 ){
		for(int i = 0 ; i < TAC ; i++)
			TagPainter( dcTemp, *TagArray.Item(i) );
		}
	if(select.selected)
		TagPainter( dcTemp, select );
#endif
	DrawCursorShadow(dcTemp);

	if(ThinSeparationLines.Count() > 0)
		for( unsigned i=0 ; i < ThinSeparationLines.Count() ; i++)
			DrawSeperationLineAfterChar( dcTemp, ThinSeparationLines.Item(i) );

	return dcTemp;
}

inline void wxHexCtrl::DrawCursorShadow(wxDC* dcTemp){
	if( m_Window.x <= 0 ||
		FindFocus()==this)
		return;

	int y=m_CharSize.y*( m_Caret.y ) + m_Margin.y;
	int x=m_CharSize.x*( m_Caret.x ) + m_Margin.x;

	dcTemp->SetPen( *wxBLACK_PEN );
	dcTemp->SetBrush( *wxTRANSPARENT_BRUSH );
	dcTemp->DrawRectangle(x,y,m_CharSize.x*2+1,m_CharSize.y);
	}

void wxHexCtrl::DrawSeperationLineAfterChar( wxDC* dcTemp, int seperationoffset ){
#ifdef _DEBUG_
		std::cout << "DrawSeperatıonLineAfterChar(" <<  seperationoffset << ")" << std::endl;
#endif

	if(m_Window.x > 0){
		wxPoint z = InternalPositionToVisibleCoord( seperationoffset );
		int y1=m_CharSize.y*( 1+z.y )+ m_Margin.y;
		int y2=y1-m_CharSize.y;
		int x1=m_CharSize.x*(z.x)+m_Margin.x;
		int x2=m_CharSize.x*2*m_Window.x+m_Margin.x;

		dcTemp->SetPen( *wxRED_PEN );
		dcTemp->DrawLine( 0,y1,x1,y1);
		if( z.x != 0)
			dcTemp->DrawLine( x1,y1,x1,y2);
		dcTemp->DrawLine( x1,y2,x2,y2);
		}
	}

#define _Use_Graphics_Contex_x
void wxHexCtrl::RePaint( void ){
	PaintMutex.Lock();
	wxCaretSuspend cs(this);

	wxDC* dcTemp = UpdateDC(); //Prepare DC
	if( dcTemp != NULL ){
		wxClientDC dc( this ); //Not looks working on GraphicsContext
		///Directly creating contentx at dc creates flicker!
		//UpdateDC(&dc);

		//dc.DrawBitmap( *internalBufferBMP, this->GetSize().GetWidth(), this->GetSize().GetHeight() ); //This does NOT work
	#ifdef WXOSX_CARBON  //wxCarbon needs +2 patch on both axis somehow.
		dc.Blit(2, 2, this->GetSize().GetWidth(), this->GetSize().GetHeight(), dcTemp, 0, 0, wxCOPY);
	#else
		dc.Blit(0, 0, this->GetSize().GetWidth(), this->GetSize().GetHeight(), dcTemp, 0, 0, wxCOPY);
	#endif //WXOSX_CARBON

#ifdef _Use_Graphics_Contex_
		wxGraphicsContext *gc = wxGraphicsContext::Create( dc );
		if (gc){
			//gc->DrawBitmap( *internalBufferBMP, 0.0, 0.0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight());
			//gc->Flush();

			int TAC = TagArray.Count();
			if( TAC != 0 )
				for(int i = 0 ; i < TAC ; i++)
					TagPainterGC( gc, *TagArray.Item(i) );

			if(select.selected)
				TagPainterGC( gc, select );
			delete gc;
		}
		else
			std::cout << " GraphicContext returs NULL!\n";
#else

#endif //_Use_Graphics_Contex_

		///delete dcTemp;
		}
	PaintMutex.Unlock();
	}

void wxHexCtrl::OnPaint( wxPaintEvent &WXUNUSED(event) ){
	PaintMutex.Lock();
	wxDC* dcTemp = UpdateDC(); // Prepare DC
	if( dcTemp != NULL )
		{
		wxPaintDC dc( this ); //wxPaintDC because here is under native wxPaintEvent.
		//wxAutoBufferedPaintDC dc( this );
		///Directly creating contentx at dc creates flicker!
		//UpdateDC(&dc);

		//dc.DrawBitmap( *internalBufferBMP, this->GetSize().GetWidth(), this->GetSize().GetHeight() ); //This does NOT work
		dc.Blit(0, 0, this->GetSize().GetWidth(), this->GetSize().GetHeight(), dcTemp, 0, 0, wxCOPY);
#ifdef _Use_Graphics_Contex_
		wxGraphicsContext *gc = wxGraphicsContext::Create( dc );
		if (gc){
			//gc->DrawBitmap( *internalBufferBMP, 0.0, 0.0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight());
			//gc->Flush();
////			// make a path that contains a circle and some lines
			gc->SetPen( *wxRED_PEN );
			wxGraphicsPath path = gc->CreatePath();
			path.AddCircle( 50.0, 50.0, 50.0 );
			path.MoveToPoint(0.0, 50.0);
			path.AddLineToPoint(100.0, 50.0);
			path.MoveToPoint(50.0, 0.0);
			path.AddLineToPoint(50.0, 100.0 );
			path.CloseSubpath();
			path.AddRectangle(25.0, 25.0, 50.0, 50.0);
			gc->StrokePath(path);

			int TAC = TagArray.Count();
			if( TAC != 0 )
				for(int i = 0 ; i < TAC ; i++)
					TagPainterGC( gc, *TagArray.Item(i) );

			if(select.selected)
				TagPainterGC( gc, select );

			delete gc;
			}
#endif
		///delete dcTemp;
		}
	PaintMutex.Unlock();
	}

void wxHexCtrl::TagPainter( wxDC* DC, TagElement& TG ){
	//Selection Painter
	DC->SetFont( HexDefaultAttr.GetFont() );
	DC->SetTextForeground( TG.FontClrData.GetColour() );
////		DC->SetTextBackground( TG.NoteClrData.GetColour() );
	DC->SetTextBackground( TG.SoftColour( TG.NoteClrData.GetColour() ));

	//wxBrush sbrush( TG.NoteClrData.GetColour() );
	//wxBrush sbrush( wxColor((unsigned long) 0x0000000),wxBRUSHSTYLE_TRANSPARENT );

	//preparation for wxGCDC, wxGraphicsContext req for semi transparent marking.

	//wxColor a = TG.NoteClrData.GetColour();
	//a.Set( a.Red(),a.Green(),a.Blue(), 10);
	//wxBrush sbrush(wxBrush(a,wxBRUSHSTYLE_TRANSPARENT ));
	//DC->SetBrush( sbrush );
	//DC->SetBackground( sbrush );
	//DC->SetBackgroundMode( wxSOLID ); // overwrite old value

	int start = TG.start;
	int end = TG.end;

	if( start > end )
		wxSwap( start, end );

	if( start < 0 )
		start = 0;

	if ( end > ByteCapacity()*2)
		 end = ByteCapacity()*2;

// TODO (death#1#): Here problem with Text Ctrl.Use smart pointer...?
	wxPoint _start_ = InternalPositionToVisibleCoord( start );
	wxPoint _end_   = InternalPositionToVisibleCoord( end );
	wxPoint _temp_  = _start_;

#ifdef _DEBUG_PAINT_
   std::cout << "Tag paint from : " << start << " to " << end << std::endl;
#endif

	//char bux[1024];//++//
	for ( ; _temp_.y <= _end_.y ; _temp_.y++ ){
		wxString line;
		_temp_.x = ( _temp_.y == _start_.y ) ? _start_.x : 0;	//calculating local line start
		int z = ( _temp_.y == _end_.y ) ? _end_.x : m_Window.x;	// and end point
		for ( int x = _temp_.x; x < z; x++ ){					//Prepare line to write process
			if( IsDenied(x) ){
				if(x+1 < z){
					line += wxT(' ');
					//bux[x]=' ';//++//
//#if wxCHECK_VERSION(2,9,0) & defined( __WXOSX__ ) //OSX DrawText bug
//					DC->DrawText( wxString::FromAscii(' '), m_Margin.x + x*m_CharSize.x, m_Margin.y + _temp_.y * m_CharSize.y );
//#endif
					}
				continue;
				}
			//bux[x]=CharAt(start); //++//
			line += CharAt(start++);

//#if wxCHECK_VERSION(2,9,0) & defined( __WXOSX__ ) //OSX DrawText bug
//			DC->DrawText( wxString::FromAscii(ch), m_Margin.x + x*m_CharSize.x, m_Margin.y + _temp_.y * m_CharSize.y );
//#endif
			}

//#if !(wxCHECK_VERSION(2,9,0) & defined( __WXOSX__ )) //OSX DrawText bug

		///Cannot convert from the charset 'Windows/DOS OEM (CP 437)'!
//		line=wxString(bux, wxCSConv(wxFONTENCODING_CP437),  _temp_.y);
		//line=wxString(line.To8BitData(), wxCSConv(wxFONTENCODING_ALTERNATIVE),  line.Len());

		//line=CP473toUnicode(line);

		//Draw one character at a time to keep char with stable.
		if(DrawCharByChar){
			//DC->SetBrush( wxBrush(TG.NoteClrData.GetColour()));
			//DC->DrawRectangle( m_Margin.x + _temp_.x*m_CharSize.x , m_Margin.y + _temp_.y * m_CharSize.y, line.Len()*m_CharSize.x, m_CharSize.y);

//			if(gc){
//				gc->SetPen( *wxRED_PEN );
//				gc->SetBrush( wxBrush(TG.NoteClrData.GetColour()));
//				wxGraphicsPath path = gc->CreatePath();
//				path.AddRectangle(m_Margin.x + _temp_.x*m_CharSize.x , m_Margin.y + _temp_.y * m_CharSize.y, line.Len()*m_CharSize.x, m_CharSize.y);
//				gc->StrokePath(path);
//				gc->DrawRectangle( m_Margin.x + _temp_.x*m_CharSize.x , m_Margin.y + _temp_.y * m_CharSize.y, line.Len()*m_CharSize.x, m_CharSize.y);
//				}


			for( unsigned q = 0 ; q < line.Len() ;q++ )
				DC->DrawText( line[q], m_Margin.x + (_temp_.x+q)*m_CharSize.x, m_Margin.y + _temp_.y * m_CharSize.y );
			}
		else
			DC->DrawText( line, m_Margin.x + _temp_.x * m_CharSize.x,	//Write prepared line
								m_Margin.x + _temp_.y * m_CharSize.y );
//#endif
		}
//	if(gc) delete gc;
	}

void wxHexCtrl::TagPainterGC( wxGraphicsContext* gc, TagElement& TG ){
	wxGraphicsFont wxgfont = gc->CreateFont( HexDefaultAttr.GetFont(), TG.FontClrData.GetColour() ) ;
	gc->SetFont( wxgfont );
	//gc->SetTextBackground( TG.SoftColour( TG.NoteClrData.GetColour() ));

	int start = TG.start;
	int end = TG.end;

	if( start > end )
		wxSwap( start, end );

	if( start < 0 )
		start = 0;

	if ( end > ByteCapacity()*2 )
		 end = ByteCapacity()*2;

// TODO (death#1#): Here problem with Text Ctrl.Use smart pointer...?
	wxPoint _start_ = InternalPositionToVisibleCoord( start );
	wxPoint _end_   = InternalPositionToVisibleCoord( end );
	wxPoint _temp_  = _start_;

#ifdef _DEBUG_PAINT_
   std::cout << "Tag paint from : " << start << " to " << end << std::endl;
#endif
	wxColor a;
	a.SetRGBA( TG.NoteClrData.GetColour().GetRGB() | 80 << 24 );
	wxBrush sbrush(wxBrush(a, wxBRUSHSTYLE_SOLID ));
	gc->SetBrush( sbrush );
	wxGraphicsBrush gcbrush = gc->CreateBrush( sbrush );
	//gc->SetPen( *wxRED_PEN );
	//Scan for each line
	for ( ; _temp_.y <= _end_.y ; _temp_.y++ ){
		wxString line;
		_temp_.x = ( _temp_.y == _start_.y ) ? _start_.x : 0;	//calculating local line start
		int z = ( _temp_.y == _end_.y ) ? _end_.x : m_Window.x;	// and end point
		for ( int x = _temp_.x; x < z; x++ ){					//Prepare line to write process
			if( IsDenied(x) ){
				if(x+1 < z){
					line += wxT(' ');
					}
				continue;
				}
			line += CharAt(start++);
			}
		//gc->DrawRectangle( m_Margin.x + _temp_.x*m_CharSize.x , m_Margin.y + _temp_.y * m_CharSize.y, line.Len()*m_CharSize.x, m_CharSize.y);
		//gc->DrawRoundedRectangle( m_Margin.x + _temp_.x*m_CharSize.x , m_Margin.y + _temp_.y * m_CharSize.y, line.Len()*m_CharSize.x, m_CharSize.y, 0.1);
//		gc->DrawText( line, m_Margin.x + _temp_.x * m_CharSize.x,	//Write prepared line
//							m_Margin.x + _temp_.y * m_CharSize.y );

		gc->DrawText( line, m_Margin.x + _temp_.x * m_CharSize.x,	//Write prepared line
							m_Margin.x + _temp_.y * m_CharSize.y,  gcbrush );

		}
	}

bool wxHexCtrl::IsAllowedChar(const char& chr){
	return isxdigit( chr );
	}

void wxHexCtrl::OnChar( wxKeyEvent &event ){
#ifdef _DEBUG_
   std::cout << "wxHexCtrl::OnChar" << std::endl;
#endif
	switch (event.GetKeyCode()){
		case WXK_LEFT:case WXK_NUMPAD_LEFT:			PrevChar();			break;
		case WXK_RIGHT:case WXK_NUMPAD_RIGHT:		NextChar();			break;
		case WXK_UP:case WXK_NUMPAD_UP:				PrevLine();			break;
		case WXK_DOWN:case WXK_NUMPAD_DOWN:			NextLine();			break;
		case WXK_HOME:case WXK_NUMPAD_HOME:			Home();				break;
		case WXK_END:case WXK_NUMPAD_END:			End();				break;
		case WXK_RETURN:			Home( false );	NextLine();			break;
		case WXK_PAGEUP:case WXK_NUMPAD_PAGEUP:
		case WXK_PAGEDOWN:case WXK_NUMPAD_PAGEDOWN:	break;
		case WXK_DELETE:case WXK_NUMPAD_DELETE:
			if( !IsDenied() )
				Replace(GetInsertionPoint(),'0');
			else
				wxBell();
			break;
		case WXK_BACK:
			if( GetInsertionPoint()!=0 ){
				PrevChar();
				if( !IsDenied() )
					Replace(GetInsertionPoint(),'0');
				}
			else
				wxBell();
			break;
// TODO (death#3#): CTRL+X
		default:
			wxChar chr = event.GetKeyCode();
			if( IsAllowedChar(chr) && !event.AltDown() && !event.ShiftDown() && !IsDenied() ){
		// TODO (death#1#): if text selected, enter from begining!
		// TODO (death#2#): If text Selected, than  remove select first?
				select.selected=false;
				(chr>='a'&&chr<='z')?(chr-=('a'-'A')):(chr=chr);	//Upper() for Char
				WriteHex(chr);

				//CharAt(m_Caret.x, m_Caret.y) = ch;
/*				wxCaretSuspend cs(this);
                wxClientDC dc(this);
                dc.SetFont(m_font);
                dc.SetBackgroundMode(wxSOLID); // overwrite old value
                dc.DrawText(chr, m_Margin.x + m_Caret.x * m_CharSize.x,
                                m_Margin.x + m_Caret.y * m_CharSize.y );
                NextChar();
*/
				}
			else
				wxBell();
				event.Skip();
			break;

	}//switch end
//	wxYield();
//	hex_selector(event);
//	paint_selection();
	DoMoveCaret();
    }

void wxHexCtrl::ChangeSize(){
	unsigned gip = GetInsertionPoint();
	wxSize size = GetClientSize();

	m_Window.x = (size.x - 2*m_Margin.x) / m_CharSize.x;
	m_Window.y = (size.y - 2*m_Margin.x) / m_CharSize.y;
	if ( m_Window.x < 1 )
		m_Window.x = 1;
	if ( m_Window.y < 1 )
		m_Window.y = 1;

	for(int i=0 ; i < m_Window.x+1 ; i++)
		IsDeniedCache[i]=IsDenied_NoCache(i);
	CharacterPerLine( true );//Updates CPL static int

	//This Resizes internal buffer!
	CreateDC();

	RePaint();
	SetInsertionPoint( gip );

#if wxUSE_STATUSBAR
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);

    if ( frame && frame->GetStatusBar() ){
    	wxString msg;
        msg.Printf(_T("Panel size is (%d, %d)"), m_Window.x, m_Window.y);
        frame->SetStatusText(msg, 1);
    }
#endif // wxUSE_STATUSBAR
}
//--------WRITE FUNCTIONS-------------//

void wxHexCtrl::WriteHex( const wxString& value ){		//write string as hex value to current position
	Replace(GetInsertionPoint(), GetInsertionPoint()+value.Length(), value);
	}

void wxHexCtrl::WriteByte( const unsigned char& byte ){	//write string as bin value to current position
	unsigned byte_location = GetInsertionPoint()/2;
	wxString buffer;
	buffer << (byte >> 4);
	buffer << (byte & 0x0F);
	Replace( byte_location*2,byte_location*2+2,buffer );
	}

void wxHexCtrl::SetBinValue( wxString buffer, bool repaint ){
	m_text.Clear();
	for( unsigned i=0 ; i < buffer.Length() ; i++ )
		m_text += wxString::Format(wxT("%02X"), static_cast<unsigned char>(buffer.at(i)));
	if(repaint)
		RePaint();
	}

void wxHexCtrl::SetBinValue( char* buffer, int byte_count, bool repaint ){
	m_text.Clear();
	for( int i=0 ; i < byte_count ; i++ )
		m_text += wxString::Format(wxT("%02X"), static_cast<unsigned char>(buffer[i]));
	if(repaint)
		RePaint();
	}

wxString wxHexCtrl::GetValue( void ){
	return m_text;
	}

void wxHexCtrl::Replace(unsigned hex_location, const wxChar& value, bool repaint){
	if( hex_location < m_text.Length() )
		m_text[hex_location] = value;
	else{
		m_text << value;
		m_text << wxT("0");
		}
	if(repaint)
		RePaint();
	}

void wxHexCtrl::Replace(unsigned from, unsigned to, const wxString& value){
	if( from >= to ) return;
// TODO (death#4#): IsHEX?
	if (!value.IsEmpty()){
// TODO (death#4#): Optimization available with use direct buffer copy
		for( int i = 0; static_cast<unsigned>(i) < value.Length() && from < to ; i++,from++ ){
			Replace( from, value[i], false );
/*			if( GetByteCount() <= from+i )	//add new hex char
				m_text << value[i];
			else						//replace old hex
				m_text[from+i] = value[i];
*/
			}
		SetInsertionPoint( to );
/*				wxCaretSuspend cs(this);
                wxClientDC dc(this);
                dc.SetFont(m_font);
                dc.SetBackgroundMode(wxSOLID); // overwrite old value
                dc.DrawText(chr, m_Margin.x + m_Caret.x * m_CharSize.x,
                                m_Margin.x + m_Caret.y * m_CharSize.y );
                NextChar();
*/
		RePaint();
		}
	else
		wxBell();
	}

char wxHexCtrl::ReadByte( int byte_location ){
	wxString hx;
	hx << m_text[ byte_location*2 ] << m_text[ byte_location*2+1 ];
	return static_cast<char*>(HexToBin(hx).GetData())[0];
	}

int atoh(const char hex){
	return ( hex >= '0' && hex <= '9' ) ? hex -'0' :
			( hex >= 'a' && hex <= 'f' ) ? hex -'a' + 10:
			( hex >= 'A' && hex <= 'F' ) ? hex -'A' + 10:
			-1;
			}

wxMemoryBuffer wxHexCtrl::HexToBin(const wxString& HexValue){
	wxMemoryBuffer memodata;
	memodata.SetBufSize(HexValue.Length()/3+1);
	char bfrL, bfrH;
	for(unsigned int i=0 ; i < HexValue.Length() ; i+=2){
		if( HexValue[i] == ' ' || HexValue[i] == ',' ){	//Removes space and period chars.
			i--; //Means +1 after loop increament of +2. Don't put i++ due HexValue.Length() check
			continue;
			}
		else if ((HexValue[i] == '0' || HexValue[i] == '\\') && ( HexValue[i+1] == 'x' || HexValue[i+1] == 'X')){ //Removes "0x", "0X", "\x", "\X"  strings.
			continue; //Means +2 by loop increament.
			}
		bfrH = atoh( HexValue[i] );
		bfrL = atoh( HexValue[i+1] );
		//Check for if it's Hexadecimal
		if( !(bfrH < 16 && bfrL < 16 && bfrH >= 0 && bfrL >= 0 )){
				wxBell();
				return memodata;
			}
		bfrL = bfrH << 4 | bfrL;
		memodata.AppendByte( bfrL );
		}
	return memodata;
	}
//------------EVENT HANDLERS---------------//
void wxHexCtrl::OnFocus(wxFocusEvent& event ){
#ifdef _DEBUG_
	std::cout << "wxHexCtrl::OnFocus()" << std::endl;
#endif
	wxCaret *caret = GetCaret();
#ifdef _DEBUG_CARET_
	std::cout << "Caret = 0x"<< (intptr_t) caret <<  " - mycaret= 0x" << (intptr_t) mycaret << "m_charSize.x" << m_CharSize.x << std::endl;
#endif
   if ( caret )
		caret->Show(true);
	}

void wxHexCtrl::OnKillFocus(wxFocusEvent& event ){
#ifdef _DEBUG_
	std::cout << "wxHexCtrl::OnKillFocus()" << std::endl;
#endif
	wxCaretSuspend cs(this);
	wxCaret *caret = GetCaret();
   if ( caret )
		caret->Show(false);

	if( TagMutex!=NULL )
		if( *TagMutex ){
			for( unsigned i = 0 ; i < TagArray.Count() ; i++ )
				TagArray.Item(i)->Hide();
			*TagMutex = false;
			}

	event.Skip();
	}

void wxHexCtrl::OnSize( wxSizeEvent &event ){
#ifdef _DEBUG_SIZE_
		std::cout << "wxHexCtrl::OnSize X,Y" << event.GetSize().GetX() <<',' << event.GetSize().GetY() << std::endl;
#endif
	ChangeSize();
	event.Skip();
	}

void wxHexCtrl::OnMouseMove( wxMouseEvent& event ){
#ifdef _DEBUG_MOUSE_
	std::cout << "wxHexCtrl::OnMouseMove Coordinate X:Y = " << event.m_x	<< " " << event.m_y
			<< "\tLMR mouse button:" << event.m_leftDown << event.m_middleDown << event.m_rightDown << std::endl;
#endif
	if(event.m_leftDown){
		select.end = PixelCoordToInternalPosition( event.GetPosition() );
		SetInsertionPoint( select.end );
		if(select.start != select.end)
			select.selected = true;
		else
			select.selected = false;
#ifdef _DEBUG_SELECT_
		std::cout << "wxHexCtrl::Selection is " << (select.selected?"true":"false") << " from " << select.start << " to " << select.end << std::endl;
#endif
		RePaint();
		}
	else{
		unsigned TagDetect = PixelCoordToInternalPosition( event.GetPosition() );
		TagElement *TAX;
		for( unsigned i = 0 ; i < TagArray.Count() ; i++ ){
			TAX = TagArray.Item(i);
			if( (TagDetect >= TAX->start ) && (TagDetect < TAX->end ) ){	//end not included!
				if( !(*TagMutex) && wxTheApp->IsActive() ) {
					*TagMutex=true;
					TAX->Show( this->ClientToScreen(event.GetPosition() ) , this );
					}
				break;
				}
			}
		}
	}
TagElement* wxHexCtrl::GetTagByPix( wxPoint PixPos ){
	unsigned TagDetect = PixelCoordToInternalPosition( PixPos );
	TagElement *TAX;
	for( unsigned i = 0 ; i < TagArray.Count() ; i++ ){
		TAX = TagArray.Item(i);
		if( (TagDetect >= TAX->start ) && (TagDetect < TAX->end ) )
			return TagArray.Item(i);
		}
	return NULL;
	}

void wxHexCtrl::OnMouseLeft( wxMouseEvent& event ){
	SetInsertionPoint( PixelCoordToInternalPosition( event.GetPosition() ) );
	select.start=GetInsertionPoint();
	}

void wxHexCtrl::OnMouseRight( wxMouseEvent& event ){
	event.Skip();
	LastRightClickPosition = event.GetPosition();
	ShowContextMenu( LastRightClickPosition );
	}

void wxHexCtrl::ShowContextMenu( wxPoint pos ){
	wxMenu menu;

	unsigned TagPosition = PixelCoordToInternalPosition( pos );
	TagElement *TAG;
	for( unsigned i = 0 ; i < TagArray.Count() ; i++ ){
		TAG = TagArray.Item(i);
		if( (TagPosition >= TAG->start ) && (TagPosition < TAG->end ) ){	//end not included!
			menu.Append(__idTagEdit__, _T("Tag Edit"));
			break;
			}
		}

	if( select.selected ){
		menu.Append(__idTagAddSelect__, _T("Tag Selection"));
		}
//  menu.AppendSeparator();
    PopupMenu(&menu, pos);
    // test for destroying items in popup menus
#if 0 // doesn't work in wxGTK!
    menu.Destroy(Menu_Popup_Submenu);
    PopupMenu( &menu, event.GetX(), event.GetY() );
#endif // 0
	}

void wxHexCtrl::OnTagEdit( wxCommandEvent& event ){
	TagElement *TAG;
	unsigned pos = PixelCoordToInternalPosition( LastRightClickPosition );
	for( unsigned i = 0 ; i < TagArray.Count() ; i++ ){
		TAG = TagArray.Item(i);
		if( TAG->isCover( pos ) ){
			TAG->Hide();	//Hide first, or BUG by double hide...
			TagElement TAGtemp = *TAG;
			TagDialog *x=new TagDialog( TAGtemp, this );
			switch( x->ShowModal() ){
				case wxID_SAVE:
					*TAG = TAGtemp;
					break;
				case wxID_DELETE:
					{
					delete TAG;
					TagArray.Remove(TAG);
					}
					break;
				default:
					break;
				}
			}
		}
	}

void wxHexCtrl::OnTagAddSelection( wxCommandEvent& event ){
	if(select.selected){
		TagElement *TAG = new TagElement;
		TAG->start=select.start;
		TAG->end=select.end;
		TagDialog *x=new TagDialog( *TAG, this );
		if( x->ShowModal() == wxID_SAVE)
			TagArray.Add( TAG );
		else
			delete TAG;
		x->Destroy();
		}
	}

void wxHexCtrl::OnTagHideAll( void ){
	for( unsigned i = 0 ; i < TagArray.Count() ; i++ )
		TagArray.Item(i)->Hide();
	}

void wxHexCtrl::OnTestCall( void ){
	wxBell();
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);
    wxString msg;
    if ( frame && frame->GetStatusBar() )
    switch(1){
		case 0:{	// ToVisiblePosition & ToInternalPosition fx test case
			for(unsigned int i = 0 ; i < m_text.Length() ; i++)
				if( ToInternalPosition(i) != ToInternalPosition(ToVisiblePosition(ToInternalPosition(i)))){
					msg.Printf(_T("To[Visible/Internal]Position fx test false at: %d"), i);
					break;
					}
				std::cout << "To[Visible/Internal]Position fx test success" << std::endl;
			break;
			}
		case 1:{	// SetInsertionPoint & GetInsertionPoint fx test case
			for (int i = 0 ; i < GetLastPosition() ; i++ ){
				SetInsertionPoint(i);
				if( i != GetInsertionPoint() )
					std::cout << "[Set/Get]InsertionPoint false at: " <<  GetInsertionPoint() << i  << std::endl;
				}
			std::cout << "[Set/Get]InsertionPoint fx test success" << std::endl;
			break;
			}
		case 2:{
			char x = 0;
			WriteByte(x);
			msg.Empty();
			msg << _T("ReadByte/WriteByte: ");
			if( x == ReadByte(0) )
				msg << _T("OK");
			else
				msg << _T("FAILED");
			frame->SetStatusText(msg, 0);
			}
			break;
		case 5:{
			//SetStyle(4,5,HexSelectAttr);
			break;
			}
		case 6:{
			char x[] = "0123456789000000";
			SetBinValue(x,16);
			/*
			msg.Empty();
			msg << _("ReadByte/WriteByte: ");
			if( x == ReadByte(0) )
				msg << _("OK");
			else
				msg << _("FAILED");
			frame->SetStatusText(msg, 0);
			*/
			break;
			}
		}
	}

///------HEXTEXTCTRL-----///
/*
inline wxChar CP437toUnicodeCHR( const unsigned char& chr){
	return CP437Table[chr];
	}

inline wxString CP437toUnicode( wxString& line ){
	wxString ret;
	for(unsigned i=0; i < line.Len() ; i++)
		ret+=CP437Table[line[i]];
	return ret;
	}
*/
inline wxChar wxHexTextCtrl::Filter(const unsigned char& ch){
	return CodepageTable[ch];
	}

inline wxString wxHexTextCtrl::FilterMBBuffer( const char *str, int Len, int fontenc ){
	wxString ret;
	//wxCSConv mcv(wxFONTENCODING_UTF8);
	// size_t WC2MB(char* buf, const wchar_t* psz, size_t n) const
	// size_t MB2WC(wchar_t* buf, const char* psz, size_t n) const
	wxString z;
	if(fontenc==wxFONTENCODING_UTF8)
		for( int i=0 ; i< Len ; i++){
			unsigned char ch = str[i];
			if(ch < 0x20) ret+='.';									// Control characters
			else if( ch >= 0x20 && ch <= 0x7E ) ret+=ch;	// ASCII compatible part
			else if( ch == 0x7F) ret+='.';						// Control character
			else if( ch >= 0x80 && ch < 0xC0 ) ret+='.';	// 6 Bit Extension Region
			else if( ch >= 0xC0 && ch < 0xC2 ) ret+='.';	// Invalid UTF8 2 byte codes
			else if( ch >= 0xC2 && ch < 0xE0 ) {				// 2 Byte UTF Start Codes
				z=wxString::FromUTF8( str+i, 2);
				//z=wxString( str+i, wxCSConv(wxFONTENCODING_UTF8), 2);
				if( z.Len() > 0){
					ret+=z;
					ret+=' ';
					i+=1;
					}
				else
					ret+='.';
				}
			else if( ch >= 0xE0 && ch < 0xF0 ){				// 3 Byte UTF Start Codes
				z=wxString::FromUTF8( str+i, 3);
				if( z.Len() > 0){
					ret+=z;
					ret+=wxT("  ");
					i+=2;
					}
				else
					ret+='.';
				}
			else if( ch >= 0xF0 && ch < 0xF5 ){				// 4 Byte UTF Start Codes
	//			ret+=wxString(str+i, wxConvUTF8, 4);
				z=wxString::FromUTF8( str+i, 4);
				if( z.Len() > 0){
					ret+=z;
					ret+=wxT("   ");
					i+=3;
					}
				else
					ret+='.';
				}
			else if( ch >= 0xF5 && ch <=0xFF ) ret+='.'; // Invalid UTF8 4 byte codes
			}
/*
	else if(fontenc==wxFONTENCODING_UTF16){
		for( int i=0 ; i< Len-1 ; i+=2){
			z=wxString ( str+i, wxCSConv(wxFONTENCODING_UTF16), 2);
			if(!(str[i]==0 && str[i+1]==0))
				ret+=z[0];
			else
				ret+=wxT(" ");
			}
		}
*/
	else if(fontenc==wxFONTENCODING_UTF16)
		for( int i=0 ; i< Len-1 ; i+=2)
			ret+=wxString( str+i, wxCSConv(wxFONTENCODING_UTF16), 2);

	else if(fontenc==wxFONTENCODING_UTF16LE)
		for( int i=0 ; i< Len-1 ; i+=2)
			ret+=wxString( str+i, wxCSConv(wxFONTENCODING_UTF16LE), 2);

	else if(fontenc==wxFONTENCODING_UTF16BE)
		for( int i=0 ; i< Len-1 ; i+=2)
			ret+=wxString( str+i, wxCSConv(wxFONTENCODING_UTF16BE), 2);

	else if(fontenc==wxFONTENCODING_UTF32)
		for( int i=0 ; i< Len-3 ; i+=4)
			ret+=wxString( str+i, wxCSConv(wxFONTENCODING_UTF32), 4);

	else if(fontenc==wxFONTENCODING_UTF32LE)
		for( int i=0 ; i< Len-3 ; i+=4)
			ret+=wxString( str+i, wxCSConv(wxFONTENCODING_UTF32LE), 4);

	else if(fontenc==wxFONTENCODING_UTF32BE)
		for( int i=0 ; i< Len-3 ; i+=4)
			ret+=wxString( str+i, wxCSConv(wxFONTENCODING_UTF32BE), 4);

	else if(fontenc==wxFONTENCODING_UNICODE){
		}

	else if(fontenc==wxFONTENCODING_SHIFT_JIS){
		//wxCSConv m_SJISConv = wxCSConv(wxFONTENCODING_SHIFT_JIS); //error on linux
		wxCSConv SJISConv = wxCSConv(wxT("SHIFT-JIS"));
		for( int i=0 ; i< Len ; i++){
			unsigned char ch = str[i];
			if(ch < 0x20) ret+='.';									//Control characters
			else if( (ch >= 0x20 && ch <= 0x7E) ||			//ASCII
						(ch > 0xA0 && ch < 0xE0 )){				//One-byte JIS X 0208 character
				ret+=wxString( str+i, SJISConv, 1);
				//ret+='O';//for debug
				}
			else if(ch < 0x81 || ch >= 0xF0 || ch==0xA0) ret+='.';	// Void Characters
			else if(Len>i){ 											//First byte of a double-byte JIS X 0208 character
				ch = str[i+1];											//Fetching second character.
				if( ch>=0x40 && ch!=0x7F && ch<=0xFC ){		//Second byte of a double-byte JIS X 0208 character
					z=wxString( str+i, SJISConv, 2);
					if(z.Len()>0){
						ret += z +wxChar(0x200B); // add zero width space since it is already two characters wide
						i++;
						}
					else
						ret+='.';
					//ret+=wxString( str+i, wxCSConv(wxFONTENCODING_SHIFT_JIS), 2);
					//ret+='X';//for debug
					}
				else
					ret+='.'; // First character is fit but second doesn't. So this is void character
				}
			}
		}

	else if(fontenc==wxFONTENCODING_CP949){//"EUC-KR"
		for( int i=0 ; i< Len ; i++){
			unsigned char ch = str[i];
			if(ch>=0x21 && ch<=0x7E)	ret+=wxChar(ch);		//ASCII part
			else if(ch>=0xA1 && ch<=0xFE && Len>i ){ // KS X 1001 (G1, code set 1) is encoded as two bytes in GR (0xA1-0xFE)
				ch=str[i+1];
				if(ch>=0xA1 && ch<=0xFE){
					z=wxString( str+i, wxCSConv(wxFONTENCODING_CP949), 2);
					if(z.Len()>0){
						ret += z +wxChar(0x200B); // add zero width space since it is already two characters wide
						i++;
						}
					else
						ret+='.';
					}
				else
					ret+='.';
				}
			else
				ret+='.';
			}
		}

	else if(fontenc==wxFONTENCODING_BIG5){
//		wxCSConv Big5Conv = wxCSConv(wxFONTENCODING_BIG5); //Error on linux
		wxCSConv Big5Conv = wxCSConv(wxT("BIG5"));
//		First byte ("lead byte") 	0x81 to 0xfe (or 0xa1 to 0xf9 for non-user-defined characters)
//		Second byte 	0x40 to 0x7e, 0xa1 to 0xfe
		for( int i=0 ; i< Len ; i++){
			unsigned char ch = str[i];
			if(ch<0x20 || ch==0x7F || ch==0x80 || ch==0xFF )
				ret+='.';
			else if( (ch >= 0x81 && ch <= 0xFE) ||	//1.st byte
				 (ch >= 0xA1 && ch < 0xF9 ) ){
				ch = str[i+1];
				if((ch >= 0x40 && ch <= 0x7E) ||		//2.nd byte
					(ch >= 0xA1 && ch <= 0xFE )){
					z=wxString(str+i, Big5Conv, 2);
					if( z.Len() > 0){
						ret+=z;
						ret+=wxChar(0x200B); // add zero width space since it is already two characters wide
						i+=1;
					}
					else
						ret+='.';
					}
				else
					ret+='.';
				}
			else
				ret+=wxString(str+i, Big5Conv, 1);
			}
		}

	else if(fontenc==wxFONTENCODING_CP936 || fontenc==wxFONTENCODING_GB2312){//"GBK"
///		range 		byte 1 	byte 2 				cp 		GB 18030 GBK 1.0 	CP936 	GB 2312
///	Level GBK/1 	A1--A9 	A1--FE 				846 		728 		717 		702 		682
///	Level GBK/2 	B0--F7 	A1--FE 				6,768 	6,763 	6,763 	6,763		6,763
///	Level GBK/3 	81--A0 	40--FE except 7F 	6,080 	6,080		6,080 	6,080
///	Level GBK/4 	AA--FE 	40--A0 except 7F 	8,160 	8,160 	8,160 	8,080
///	Level GBK/5 	A8--A9 	40--A0 except 7F 	192 		166 		166 		166
///	user-defined 	AA--AF 	A1--FE 				564
///	user-defined 	F8--FE 	A1--FE 				658
///	user-defined 	A1--A7 	40--A0 except 7F 	672
///	total: 												23,940 	21,897 	21,886 	21,791 	7,445
		for( int i=0 ; i< Len ; i++){
			unsigned char ch1 = str[i];
			if(ch1>=0x21 && ch1<=0x7E)	ret+=wxChar(ch1); 	//ASCII part
			else if(ch1>=0xA1 && ch1<=0xFE && Len>i ){ 	// KS X 1001 (G1, code set 1) is encoded as two bytes in GR (0xA1-0xFE)
				unsigned char ch2=str[i+1];
		//		if(ch>=0x40 && ch<=0xFE){
				if(
					(ch1>=0xA1 && ch1<=0xA9 && ch2>=0xA1 && ch2<=0xFE ) || //GBK/1
					(ch1>=0xB0 && ch1<=0xF7 && ch2>=0xA1 && ch2<=0xFE ) || //GBK/2
					(ch1>=0x81 && ch1<=0xA0 && ch2>=0x40 && ch2<=0xFE && ch2!=0x7F) || //GBK/3
					(ch1>=0xAA && ch1<=0xFE && ch2>=0x40 && ch2<=0xA0 && ch2!=0x7F ) || //GBK/4
					(ch1>=0xA8 && ch1<=0xA9 && ch2>=0x40 && ch2<=0xA0 && ch2!=0x7F ) || //GBK/5
					(ch1>=0xAA && ch1<=0xAF && ch2>=0xA1 && ch2<=0xFE) || //user-defined
					(ch1>=0xF8 && ch1<=0xFE && ch2>=0xA1 && ch2<=0xFE) || //user-defined
					(ch1>=0xA1 && ch1<=0xA7 && ch2>=0x40 && ch2<=0xA0 && ch2!=0x7F) //user-defined
					){
					z = wxString(str + i, wxCSConv((wxFontEncoding)fontenc), 2);
					if(z.Len()>0){
						ret += z +wxChar(0x200B); // add zero width space since it is already two characters wide
						i++;
						}
					else
						ret+='.';
					}
				else
					ret+='.';
				}
			else
				ret+='.';
			}
		}

//	else if(fontenc==wxFONTENCODING_GB2312)
//		for( int i=0 ; i< Len ; i++){
//			unsigned char ch = str[i];
//			if(ch < 0x20) ret+='.';									// Control characters
//			else if( ch >= 0x20 && ch <= 0x7E ) ret+=ch;	// ASCII compatible part
//			else if(ch < 0x80) ret+='.';							// Void Characters
//			else { // 0x21 + 0x80
//				z=wxString( str+i, wxCSConv(wxFONTENCODING_GB2312), 2);
//				if( z.Len() > 0){
//					ret+=z;
//					if( z.Len() == 1 )
//						ret+=' ';
//					i+=1;
//					}
//				else
//					ret+='.';
//				}
//		}

	else if(fontenc==wxFONTENCODING_EUC_JP){
		wxCSConv EUCJPConv = wxCSConv(wxFONTENCODING_EUC_JP);
		//wxCSConv EUCJPConv = wxCSConv(wxT("EUC-JP"));

    ///A character from the lower half of JIS-X-0201 (ASCII, code set 0) is represented by one byte, in the range 0x21 – 0x7E.
    ///A character from the upper half of JIS-X-0201 (half-width kana, code set 2) is represented by two bytes, the first being 0x8E, the second in the range 0xA1 – 0xDF.
    ///A character from JIS-X-0208 (code set 1) is represented by two bytes, both in the range 0xA1 – 0xFE.
    ///A character from JIS-X-0212 (code set 3) is represented by three bytes, the first being 0x8F, the following two in the range 0xA1 – 0xFE.
		for( int i=0 ; i< Len ; i++){
			unsigned char ch = str[i];
			if(ch<0x20 || ch==0x7F )			ret+='.';				//Control chars
			else if(ch>=0x21 && ch<=0x7E)	ret+=wxChar(ch);		//ASCII part
			else if(ch==0x8E && Len > i){								//half-width kana first byte
				ch = str[i+1];
				if(ch>=0xA1 && ch<=0xDF)									//half-width kana second byte
					ret+=wxString(str+i++, EUCJPConv, 2) + wxT(" ");
				else
					ret+='.';
				}
			else if(ch>=0xA1 && ch <=0xFE){		//JIS-X-0208 first byte
				ch = str[i+1];
				if(ch>=0xA1 && ch <=0xFE){		//JIS-X-0208 second byte
					z=wxString(str+i, EUCJPConv, 2);
					if(z.Len()>0){
						ret+=z + wxT(" ");
						i++;
						}
					else
						ret+='.';
					}
				else
					ret+='.';
				}
			else if(ch==0x8F && (Len>i+1)){	//JIS-X-0212 first byte
				unsigned char ch1=str[i+1];
				unsigned char ch2=str[i+2];
				if((ch1>=0xA1 && ch1<=0xFE) &&
					(ch2>=0xA1 && ch2<=0xFE)){	//JIS-X-0212 second byte
					ret+=wxString(str+i, EUCJPConv, 3) + wxT("  ");
					i+=2;
					}
				else
					ret+='.';
				}
			else
				ret+='.';
			}
		}

	else if(Codepage.StartsWith(wxT("TSCII"))){
///		0x82 4Byte
///		0x87 3Byte
///		0x88->0x8B 2Byte
///		0x8C 4Byte
///		0x99->0x9C 2Byte
///		0xCA->0xFD 2Byte

		int p;
		for(int i=0;i<Len;i++){
			p=0;
			if(i<0x80)
				ret+=Filter(str[i]);
			else{
				if(i>0x82)	p+=3;
				if(i>0x87)	p+=2;
				if(i>0x8B)	p+=(0x8B-0x88)*1;
				if(i>0x8C)	p+=3;
				if(i>0x9C)	p+=(0x9C-0x99)*1;
				if(i>0xFD)	p+=(0xFD-0xCA)*1;

				if(i==0x82)	ret+=CodepageTable.Mid(i,4);
				else if(i==0x87)					ret+=CodepageTable.Mid(i+p,3);
				else if(i>=0x88 && i<=0x8B)	ret+=CodepageTable.Mid((i-0x88)*2+p,2);
				else if(i==0x8C)					ret+=CodepageTable.Mid(i+p,4);
				else if(i>=0x99 && i<=0x9C)	ret+=CodepageTable.Mid((i-0x99)*2+p,2);
				else if(i>=0xCA && i<=0xFD)	ret+=CodepageTable.Mid((i-0xCA)*2+p,2);
				else ret+=CodepageTable[p];
				}
			}
		}

	return ret;
	}

//wxArrayString wxHexTextCtrl::GetSupportedEncodings(void){
//	return GetSupportedEncodings(void);
//	}

wxArrayString GetSupportedEncodings(void){
	wxArrayString AvailableEncodings;
	wxString CharacterEncodings[] = {
					wxT("ASCII - American Standard Code for Information Interchange"),
					wxT("*ISCII - Indian Script Code for Information Interchange"),
					wxString::FromUTF8("KOI7 \xD0\x9A\xD0\xBE\xD0\xB4\x20\xD0\x9E\xD0\xB1\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB0\x20\xD0\x98\xD0\xBD\xD1\x84\xD0\xBE\xD1\x80\xD0\xBC\xD0\xB0\xD1\x86\xD0\xB8\xD0\xB5\xD0\xB9, 7 \xD0\xB1\xD0\xB8\xD1\x82"),
					wxString::FromUTF8("KOI8-R \xD0\x9A\xD0\xBE\xD0\xB4\x20\xD0\x9E\xD0\xB1\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB0\x20\xD0\x98\xD0\xBD\xD1\x84\xD0\xBE\xD1\x80\xD0\xBC\xD0\xB0\xD1\x86\xD0\xB8\xD0\xB5\xD0\xB9, 8 \xD0\xB1\xD0\xB8\xD1\x82"),
					wxString::FromUTF8("KOI8-U \xD0\x9A\xD0\xBE\xD0\xB4\x20\xD0\x9E\xD0\xB1\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB0\x20\xD0\x98\xD0\xBD\xD1\x84\xD0\xBE\xD1\x80\xD0\xBC\xD0\xB0\xD1\x86\xD0\xB8\xD0\xB5\xD0\xB9, 8 \xD0\xB1\xD0\xB8\xD1\x82"),
					wxT("ISO/IEC 6937"),
					wxT("ISO/IEC 8859-1 Latin-1 Western European"),
					wxT("ISO/IEC 8859-2 Latin-2 Central European"),
					wxT("ISO/IEC 8859-3 Latin-3 South European"),
					wxT("ISO/IEC 8859-4 Latin-4 North European"),
					wxT("ISO/IEC 8859-5 Latin/Cyrillic"),
					wxT("*ISO/IEC 8859-6 Latin/Arabic"),
					wxT("ISO/IEC 8859-7 Latin/Greek"),
					wxT("*ISO/IEC 8859-8 Latin/Hebrew"),
					wxT("ISO/IEC 8859-9 Latin/Turkish"),
					wxT("ISO/IEC 8859-10 Latin/Nordic"),
					wxT("*ISO/IEC 8859-11 Latin/Thai"),
					wxT("ISO/IEC 8859-13 Latin-7 Baltic Rim"),
					wxT("ISO/IEC 8859-14 Latin-8 Celtic"),
					wxT("ISO/IEC 8859-15 Latin-9"),
					wxT("ISO/IEC 8859-16 Latin-10 South-Eastern European"),
					wxT("*Windows CP874 - Thai"),
					wxT("Windows CP1250 - Central and Eastern European"),
					wxT("Windows CP1251 - Cyrillic Script"),
					wxT("Windows CP1252 - ANSI"),
					wxT("Windows CP1253 - Greek Modern"),
					wxT("Windows CP1254 - Turkish"),
					wxT("*Windows CP1255 - Hebrew"),
					wxT("*Windows CP1256 - Arabic"),
					wxT("Windows CP1257 - Baltic"),
					wxT("Windows CP1258 - Vietnamese"),
					wxT("VSCII - Vietnamese Standard Code for Information Interchange"),
					wxT("*TSCII - Tamil Script Code for Information Interchange"),
					wxT("*JIS X 0201 - Japanese Industrial Standard "),
					wxT("*TIS-620 - Thai Industrial Standard 620-2533"),
					wxT("EBCDIC  037 - IBM U.S. Canada"),
					wxT("EBCDIC  285 - IBM Ireland U.K."),
					wxT("EBCDIC  424 - IBM Hebrew"),
					wxT("EBCDIC  500 - IBM International"),
					wxT("EBCDIC  875 - IBM Greek"),
					wxT("EBCDIC 1026 - IBM Latin 5 Turkish"),
					wxT("EBCDIC 1047 - IBM Latin 1"),
					wxString::FromUTF8("EBCDIC 1140 - IBM U.S. Canada with \xE2\x82\xAC"),
					wxString::FromUTF8("EBCDIC 1146 - IBM Ireland U.K. with \xE2\x82\xAC"),
					wxString::FromUTF8("EBCDIC 1148 - IBM International with \xE2\x82\xAC"),
					wxT("*ANSEL - American National Standard for Extended Latin"),
					wxT("DEC Multinational Character Set - VT220"),
					wxT("OEM - IBM PC/DOS CP437 - MS-DOS Latin US"),
					wxT("*PC/DOS CP720 - MS-DOS Arabic"),
					wxT("PC/DOS CP737 - MS-DOS Greek"),
					wxT("PC/DOS CP775 - MS-DOS Baltic Rim"),
					wxT("PC/DOS CP850 - MS-DOS Latin 1"),
					wxT("PC/DOS CP852 - MS-DOS Latin 2"),
					wxT("PC/DOS CP855 - MS-DOS Cyrillic"),
					wxT("*PC/DOS CP856 - Hebrew"),
					wxT("PC/DOS CP857 - MS-DOS Turkish"),
					wxT("PC/DOS CP858 - MS-DOS Latin 1 Update"),
					wxT("PC/DOS CP860 - MS-DOS Portuguese"),
					wxT("PC/DOS CP861 - MS-DOS Icelandic"),
					wxT("*PC/DOS CP862 - MS-DOS Hebrew"),
					wxT("PC/DOS CP863 - MS-DOS French Canada"),
					wxT("*PC/DOS CP864 - MS-DOS Arabic 2"),
					wxT("PC/DOS CP866 - MS-DOS Cyrillic Russian"),
					wxT("PC/DOS CP869 - MS-DOS Greek 2"),
					wxT("PC/DOS CP1006 - Arabic"),
					wxT("PC/DOS KZ-1048 - Kazakhstan"),
					wxT("PC/DOS MIK Code page"),
					wxString::FromUTF8("PC/DOS Kamenick\xC3\xBD Encoding"),
					wxT("PC/DOS Mazovia Encoding"),
					wxT("*PC/DOS Iran System Encoding Standard"),
					wxT("*Big5"),
					wxString::FromUTF8("*GBK - GB2312 - Guojia Biaozhun (\xE5\x9B\xBD\xE5\xAE\xB6\xE6\xA0\x87\xE5\x87\x86)"),
					wxT("UTF8 - Universal Character Set"),
					wxT("UTF16 - Universal Character Set"),
					wxT("UTF16LE - Universal Character Set"),
					wxT("UTF16BE - Universal Character Set"),
					wxT("UTF32 - Universal Character Set"),
					wxT("UTF32LE - Universal Character Set"),
					wxT("UTF32BE - Universal Character Set"),
					wxT("Unicode"),
					wxT("*EUC-JP Extended Unix Code for Japanese"),
					wxT("*EUC-KR Extended Unix Code for Korean"),
					wxT("*Shift JIS"),

					wxT("Macintosh CP10000 - MacRoman"),
					wxT("Macintosh CP10007 - MacCyrillic"),
					wxT("Macintosh CP10006 - MacGreek"),
					wxT("Macintosh CP10079 - MacIcelandic"),
					wxT("Macintosh CP10029 - MacLatin2"),
					wxT("Macintosh CP10081 - MacTurkish"),

		#ifdef __WXMAC__
					wxT("Macintosh Arabic"),
					wxT("Macintosh Celtic"),
					wxT("Macintosh Central European"),
					wxT("Macintosh Croatian"),
					wxT("Macintosh Cyrillic"),
					wxT("Macintosh Devanagari"),
					wxT("Macintosh Dingbats"),
					wxT("Macintosh Gaelic"),
					wxT("Macintosh Greek"),
					wxT("Macintosh Gujarati"),
					wxT("Macintosh Gurmukhi"),
					wxT("Macintosh Hebrew"),
					wxT("Macintosh Icelandic"),
					wxT("Macintosh Inuit"),
					wxT("Macintosh Keyboard"),
					wxT("Macintosh Roman"),
					wxT("Macintosh Romanian"),
					wxT("Macintosh Symbol"),
					wxT("Macintosh Thai"),
					wxT("Macintosh Tibetan"),
					wxT("Macintosh Turkish"),
					wxT("Macintosh Ukraine"),
			#endif

					wxT("*AtariST"),
					wxT("*Windows CP932 - Japanese (Shift JIS)"),
					wxT("*Windows CP936 - Chinese Simplified (GBK)"),
					wxT("*Windows CP949 - Korean (EUC-KR)"),
					wxT("*Windows CP950 - Chinese Traditional (Big5)")
					};

		int EncCnt = sizeof( CharacterEncodings ) / sizeof( wxString );
		for( int i=0 ; i< EncCnt ; i++ )
			AvailableEncodings.Add(CharacterEncodings[i]);
		return AvailableEncodings;
		}

wxString wxHexTextCtrl::PrepareCodepageTable(wxString codepage){
/****Python script for fetch Code page tables*********
import urllib,sys
def cpformat( a ):
   q = urllib.urlopen( a ).read().split('\n')
   w=[i.split('\t') for i in q]
   e=[i.split('\t') for i in q if not i.startswith('#')]
   r=[i[1] for i in e if len(i)>1]
   z=0
   sys.stdout.write('wxT("')
   for i in r[0x00:]:
      if not z%0x10 and z!=0:
         sys.stdout.write('"\\\n"')
      if i=='':
         sys.stdout.write('.')
      elif i[2:4]=='00':
         sys.stdout.write('\\x'+i[4:].upper())
      else:
         sys.stdout.write('\\x'+i[2:].upper())
      z+=1
   sys.stdout.write('" );\n')

a='http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP437.TXT'
cpformat(a)
*******************************************************/
	Codepage=codepage;
	wxString newCP;
	FontEnc=wxFONTENCODING_ALTERNATIVE;
	char bf[256];
	if(codepage.Find(wxT("ASCII")) != wxNOT_FOUND)
		{
		for (unsigned i=0; i<=0xFF ; i++){
			if(i<0x20 || i>=0x7F)		newCP+='.';		  //Control chars replaced with dot
			if(i>=0x20 && i<0x7F)		newCP+=wxChar(i);//ASCII region
			}
		}
	else if(codepage.Find(wxT("ANSEL")) != wxNOT_FOUND){
		for (unsigned i=0; i<0xA1 ; i++){
			if(i<0x20 || i>=0x7F)		newCP+='.';		  //Control chars and voids replaced with dot
			if(i>=0x20 && i<0x7F)		newCP+=wxChar(i);//ASCII region
			}
		newCP += wxString::FromUTF8("\xC5\x81\xC3\x98\xC4\x90\xC3\x9E\xC3\x86\xC5\x92\xCA\xB9\xC2\xB7\xE2\x99\xAD\xC2\xAE\xC2\xB1\xC6\xA0\xC6\xAF\xCA\xBC."
			"\xCA\xBB\xC5\x82\xC3\xB8\xC4\x91\xC3\xBE\xC3\xA6\xC5\x93\xCA\xBA\xC4\xB1\xC2\xA3\xC3\xB0.\xC6\xA1\xC6\xB0.."
			"\xC2\xB0\xE2\x84\x93\xE2\x84\x97\xC2\xA9\xE2\x99\xAF\xC2\xBF\xC2\xA1");

		for (unsigned i=0xC7; i<0xE0 ; i++) newCP+='.';//Void Region
		newCP += wxString::FromUTF8("\xCC\x83\xCC\x80\xCC\x81\xCC\x82\xCC\x83\xCC\x84\xCC\x86\xCC\x87\xCC\x88\xCC\x8C\xCC\x8A\xEF\xB8\xA0\xEF\xB8\xA1\xCC\x95\xCC\x8B"
			"\xCC\x90\xCC\xA7\xCC\xA8\xCC\xA3\xCC\xA4\xCC\xA5\xCC\xB3\xCC\xB2\xCC\xA6\xCC\x9C\xCC\xAE\xEF\xB8\xA2\xEF\xB8\xA3..\xCC\x93");
	}

	//Indian Script Code for Information Interchange
	else if(codepage.Find(wxT("ISCII")) != wxNOT_FOUND){
		for (unsigned i=0; i<=0xA1 ; i++)
			newCP+=wxChar((i<0x20 || i>=0x7F) ? '.' : i);
		//Unicode eq of 0xD9 is \x25CC || \x00AD
		newCP += wxString::FromUTF8("\xE0\xA4\x81\xE0\xA4\x82\xE0\xA4\x83\xE0\xA4\x85\xE0\xA4\x86\xE0\xA4\x87\xE0\xA4\x88\xE0\xA4\x89\xE0\xA4\x8A\xE0\xA4\x8B"
			"\xE0\xA4\x8E\xE0\xA4\x8F\xE0\xA4\x90\xE0\xA4\x8D\xE0\xA4\x92\xE0\xA4\x93\xE0\xA4\x94\xE0\xA4\x91\xE0\xA4\x95\xE0\xA4\x96"
			"\xE0\xA4\x97\xE0\xA4\x98\xE0\xA4\x99\xE0\xA4\x9A\xE0\xA4\x9B\xE0\xA4\x9C\xE0\xA4\x9D\xE0\xA4\x9E\xE0\xA4\x9F\xE0\xA4\xA0"
			"\xE0\xA4\xA1\xE0\xA4\xA2\xE0\xA4\xA3\xE0\xA4\xA4\xE0\xA4\xA5\xE0\xA4\xA6\xE0\xA4\xA7\xE0\xA4\xA8\xE0\xA4\xA9\xE0\xA4\xAA"
			"\xE0\xA4\xAB\xE0\xA4\xAC\xE0\xA4\xAD\xE0\xA4\xAE\xE0\xA4\xAF\xE0\xA5\x9F\xE0\xA4\xB0\xE0\xA4\xB1\xE0\xA4\xB2\xE0\xA4\xB3"
			"\xE0\xA4\xB4\xE0\xA4\xB5\xE0\xA4\xB6\xE0\xA4\xB7\xE0\xA4\xB8\xE0\xA4\xB9\xE2\x97\x8C\xE0\xA4\xBE\xE0\xA4\xBF\xE0\xA5\x80"
			"\xE0\xA5\x81\xE0\xA5\x82\xE0\xA5\x83\xE0\xA5\x86\xE0\xA5\x87\xE0\xA5\x88\xE0\xA5\x85\xE0\xA5\x8A\xE0\xA5\x8B\xE0\xA5\x8C"
			"\xE0\xA5\x89\xE0\xA5\x8D\xE0\xA4\xBC\xE0\xA5\xA4......\xE0\xA5\xA6\xE0\xA5\xA7\xE0\xA5\xA8"
			"\xE0\xA5\xA9\xE0\xA5\xAA\xE0\xA5\xAB\xE0\xA5\xAC\xE0\xA5\xAD\xE0\xA5\xAE\xE0\xA5\xAF");
	}

	//Tamil Script Code for Information Interchange
	else if(codepage.Find(wxT("TSCII")) != wxNOT_FOUND){
		newCP=PrepareCodepageTable(wxT("ASCII")).Mid(0,0x80);
		///		0x82 4Byte
		///		0x87 3Byte
		///		0x88->0x8B 2Byte
		///		0x8C 4Byte
		///		0xCA->0xFD 2Byte
		///		0x99->0x9C 2Byte
		newCP += wxString::FromUTF8("\xE0\xAF\xA6\xE0\xAF\xA7\xE0\xAE\xB8\xE0\xAF\x8D\xE0\xAE\xB0\xE0\xAF\x80\xE0\xAE\x9C\xE0\xAE\xB7\xE0\xAE\xB8\xE0\xAE\xB9\xE0\xAE\x95"
			"\xE0\xAF\x8D\xE0\xAE\xB7\xE0\xAE\x9C\xE0\xAF\x8D\xE0\xAE\xB7\xE0\xAF\x8D\xE0\xAE\xB8\xE0\xAF\x8D\xE0\xAE\xB9\xE0\xAF\x8D\xE0\xAE\x95"
			"\xE0\xAF\x8D\xE0\xAE\xB7\xE0\xAF\x8D\xE0\xAF\xA8\xE0\xAF\xA9\xE0\xAF\xAA\xE0\xAF\xAB\xE2\x80\x98\xE2\x80\x99\xE2\x80\x9C\xE2\x80\x9D"
			"\xE0\xAF\xAC\xE0\xAF\xAD\xE0\xAF\xAE\xE0\xAF\xAF\xE0\xAE\x99\xE0\xAF\x81\xE0\xAE\x9E\xE0\xAF\x81\xE0\xAE\x99\xE0\xAF\x82\xE0\xAE\x9E"
			"\xE0\xAF\x82\xE0\xAF\xB0\xE0\xAF\xB1\xE0\xAF\xB2\xC2\xA0\xE0\xAE\xBE\xE0\xAE\xBF\xE0\xAF\x80\xE0\xAF\x81\xE0\xAF\x82\xE0\xAF\x86"
			"\xE0\xAF\x87\xE0\xAF\x88\xC2\xA9\xE0\xAF\x97\xE0\xAE\x85\xE0\xAE\x86.\xE0\xAE\x88\xE0\xAE\x89\xE0\xAE\x8A\xE0\xAE\x8E\xE0\xAE\x8F"
			"\xE0\xAE\x90\xE0\xAE\x92\xE0\xAE\x93\xE0\xAE\x94\xE0\xAE\x83\xE0\xAE\x95\xE0\xAE\x99\xE0\xAE\x9A\xE0\xAE\x9E\xE0\xAE\x9F\xE0\xAE\xA3"
			"\xE0\xAE\xA4\xE0\xAE\xA8\xE0\xAE\xAA\xE0\xAE\xAE\xE0\xAE\xAF\xE0\xAE\xB0\xE0\xAE\xB2\xE0\xAE\xB5\xE0\xAE\xB4\xE0\xAE\xB3\xE0\xAE\xB1"
			"\xE0\xAE\xA9\xE0\xAE\x9F\xE0\xAE\xBF\xE0\xAE\x9F\xE0\xAF\x80\xE0\xAE\x95\xE0\xAF\x81\xE0\xAE\x9A\xE0\xAF\x81\xE0\xAE\x9F\xE0\xAF\x81"
			"\xE0\xAE\xA3\xE0\xAF\x81\xE0\xAE\xA4\xE0\xAF\x81\xE0\xAE\xA8\xE0\xAF\x81\xE0\xAE\xAA\xE0\xAF\x81\xE0\xAE\xAE\xE0\xAF\x81\xE0\xAE\xAF"
			"\xE0\xAF\x81\xE0\xAE\xB0\xE0\xAF\x81\xE0\xAE\xB2\xE0\xAF\x81\xE0\xAE\xB5\xE0\xAF\x81\xE0\xAE\xB4\xE0\xAF\x81\xE0\xAE\xB3\xE0\xAF\x81"
			"\xE0\xAE\xB1\xE0\xAF\x81\xE0\xAE\xA9\xE0\xAF\x81\xE0\xAE\x95\xE0\xAF\x82\xE0\xAE\x9A\xE0\xAF\x82\xE0\xAE\x9F\xE0\xAF\x82\xE0\xAE\xA3"
			"\xE0\xAF\x82\xE0\xAE\xA4\xE0\xAF\x82\xE0\xAE\xA8\xE0\xAF\x82\xE0\xAE\xAA\xE0\xAF\x82\xE0\xAE\xAE\xE0\xAF\x82\xE0\xAE\xAF\xE0\xAF\x82"
			"\xE0\xAE\xB0\xE0\xAF\x82\xE0\xAE\xB2\xE0\xAF\x82\xE0\xAE\xB5\xE0\xAF\x82\xE0\xAE\xB4\xE0\xAF\x82\xE0\xAE\xB3\xE0\xAF\x82\xE0\xAE\xB1"
			"\xE0\xAF\x82\xE0\xAE\xA9\xE0\xAF\x82\xE0\xAE\x95\xE0\xAF\x8D\xE0\xAE\x99\xE0\xAF\x8D\xE0\xAE\x9A\xE0\xAF\x8D\xE0\xAE\x9E\xE0\xAF\x8D"
			"\xE0\xAE\x9F\xE0\xAF\x8D\xE0\xAE\xA3\xE0\xAF\x8D\xE0\xAE\xA4\xE0\xAF\x8D\xE0\xAE\xA8\xE0\xAF\x8D\xE0\xAE\xAA\xE0\xAF\x8D\xE0\xAE\xAE"
			"\xE0\xAF\x8D\xE0\xAE\xAF\xE0\xAF\x8D\xE0\xAE\xB0\xE0\xAF\x8D\xE0\xAE\xB2\xE0\xAF\x8D\xE0\xAE\xB5\xE0\xAF\x8D\xE0\xAE\xB4\xE0\xAF\x8D"
			"\xE0\xAE\xB3\xE0\xAF\x8D\xE0\xAE\xB1\xE0\xAF\x8D\xE0\xAE\xA9\xE0\xAF\x8D\xE0\xAE\x87");
	}

	else if(codepage.Find(wxT("VSCII")) != wxNOT_FOUND){
		for (unsigned i=0; i<=0x7F ; i++)
			if (i == 0x02) newCP += wxString::FromUTF8("\xE1\xBA\xB2");
			else if (i == 0x05) newCP += wxString::FromUTF8("\xE1\xBA\xB4");
			else if (i == 0x06) newCP += wxString::FromUTF8("\xE1\xBA\xAA");
			else if (i == 0x14) newCP += wxString::FromUTF8("\xE1\xBB\xB6");
			else if (i == 0x19) newCP += wxString::FromUTF8("\xE1\xBB\xB8");
			else if (i == 0x1E) newCP += wxString::FromUTF8("\xE1\xBB\xB4");
			else newCP += wxChar((i<0x20 || i >= 0x7F) ? '.' : i);

			newCP += wxString::FromUTF8("\xE1\xBA\xA0\xE1\xBA\xAE\xE1\xBA\xB0\xE1\xBA\xB6\xE1\xBA\xA4\xE1\xBA\xA6\xE1\xBA\xA8\xE1\xBA\xAC\xE1\xBA\xBC\xE1\xBA\xB8"
				"\xE1\xBA\xBE\xE1\xBB\x80\xE1\xBB\x82\xE1\xBB\x84\xE1\xBB\x86\xE1\xBB\x90\xE1\xBB\x92\xE1\xBB\x94\xE1\xBB\x96\xE1\xBB\x98"
				"\xE1\xBB\xA2\xE1\xBB\x9A\xE1\xBB\x9C\xE1\xBB\x9E\xE1\xBB\x8A\xE1\xBB\x8E\xE1\xBB\x8C\xE1\xBB\x88\xE1\xBB\xA6\xC5\xA8"
				"\xE1\xBB\xA4\xE1\xBB\xB2\xC3\x95\xE1\xBA\xAF\xE1\xBA\xB1\xE1\xBA\xB7\xE1\xBA\xA5\xE1\xBA\xA7\xE1\xBA\xA9\xE1\xBA\xAD"
				"\xE1\xBA\xBD\xE1\xBA\xB9\xE1\xBA\xBF\xE1\xBB\x81\xE1\xBB\x83\xE1\xBB\x85\xE1\xBB\x87\xE1\xBB\x91\xE1\xBB\x93\xE1\xBB\x95"
				"\xE1\xBB\x97\xE1\xBB\xA0\xC6\xA0\xE1\xBB\x99\xE1\xBB\x9D\xE1\xBB\x9F\xE1\xBB\x8B\xE1\xBB\xB0\xE1\xBB\xA8\xE1\xBB\xAA"
				"\xE1\xBB\xAC\xC6\xA1\xE1\xBB\x9B\xC6\xAF\xC3\x80\xC3\x81\xC3\x82\xC3\x83\xE1\xBA\xA2\xC4\x82"
				"\xE1\xBA\xB3\xE1\xBA\xB5\xC3\x88\xC3\x89\xC3\x8A\xE1\xBA\xBA\xC3\x8C\xC3\x8D\xC4\xA8\xE1\xBB\xB3"
				"\xC4\x90\xE1\xBB\xA9\xC3\x92\xC3\x93\xC3\x94\xE1\xBA\xA1\xE1\xBB\xB7\xE1\xBB\xAB\xE1\xBB\xAD\xC3\x99"
				"\xC3\x9A\xE1\xBB\xB9\xE1\xBB\xB5\xC3\x9D\xE1\xBB\xA1\xC6\xB0\xC3\xA0\xC3\xA1\xC3\xA2\xC3\xA3"
				"\xE1\xBA\xA3\xC4\x83\xE1\xBB\xAF\xE1\xBA\xAB\xC3\xA8\xC3\xA9\xC3\xAA\xE1\xBA\xBB\xC3\xAC\xC3\xAD"
				"\xC4\xA9\xE1\xBB\x89\xC4\x91\xE1\xBB\xB1\xC3\xB2\xC3\xB3\xC3\xB4\xC3\xB5\xE1\xBB\x8F\xE1\xBB\x8D"
				"\xE1\xBB\xA5\xC3\xB9\xC3\xBA\xC5\xA9\xE1\xBB\xA7\xC3\xBD\xE1\xBB\xA3\xE1\xBB\xAE");
	}

	// OEM PC/DOS
	else if(codepage.Find(wxT("DOS")) != wxNOT_FOUND ){
		//CP437 Control Symbols
		newCP = wxString::FromUTF8("\x20\xE2\x98\xBA\xE2\x98\xBB\xE2\x99\xA5\xE2\x99\xA6\xE2\x99\xA3\xE2\x99\xA0\xE2\x80\xA2\xE2\x97\x98\xE2\x97\x8B\xE2\x97\x99"
			"\xE2\x99\x82\xE2\x99\x80\xE2\x99\xAA\xE2\x99\xAB\xE2\x98\xBC\xE2\x96\xBA\xE2\x97\x84\xE2\x86\x95\xE2\x80\xBC\xC2\xB6\xC2\xA7"
			"\xE2\x96\xAC\xE2\x86\xA8\xE2\x86\x91\xE2\x86\x93\xE2\x86\x92\xE2\x86\x90\xE2\x88\x9F\xE2\x86\x94\xE2\x96\xB2\xE2\x96\xBC");

		//ASCII compatible part
		for( unsigned i=0x20 ; i < 0x7F ; i++ )
			newCP += wxChar(i);

		newCP+=wxChar(0x2302); //0x7F symbol

		for (unsigned i=0x80; i<=0xFF ; i++)
			bf[i-0x80] =  wxChar(i);

		if ((codepage.Find(wxT("CP437")) != wxNOT_FOUND) || //Extended ASCII region of CP437
			(codepage.Find(wxT("OEM")) != wxNOT_FOUND))
			//			newCP+=wxString( bf, wxCSConv(wxT("CP437")), 0x80);
			newCP += wxString::FromUTF8("\xC3\x87\xC3\xBC\xC3\xA9\xC3\xA2\xC3\xA4\xC3\xA0\xC3\xA5\xC3\xA7\xC3\xAA\xC3\xAB\xC3\xA8\xC3\xAF\xC3\xAE\xC3\xAC\xC3\x84\xC3\x85"
			"\xC3\x89\xC3\xA6\xC3\x86\xC3\xB4\xC3\xB6\xC3\xB2\xC3\xBB\xC3\xB9\xC3\xBF\xC3\x96\xC3\x9C\xC2\xA2\xC2\xA3\xC2\xA5\xE2\x82\xA7\xC6\x92"
			"\xC3\xA1\xC3\xAD\xC3\xB3\xC3\xBA\xC3\xB1\xC3\x91\xC2\xAA\xC2\xBA\xC2\xBF\xE2\x8C\x90\xC2\xAC\xC2\xBD\xC2\xBC\xC2\xA1\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2\x95\xA1\xE2\x95\xA2\xE2\x95\x96\xE2\x95\x95\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xE2\x95\x9C\xE2\x95\x9B\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xE2\x95\x9E\xE2\x95\x9F\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x95\xA7"
			"\xE2\x95\xA8\xE2\x95\xA4\xE2\x95\xA5\xE2\x95\x99\xE2\x95\x98\xE2\x95\x92\xE2\x95\x93\xE2\x95\xAB\xE2\x95\xAA\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
			"\xCE\xB1\xC3\x9F\xCE\x93\xCF\x80\xCE\xA3\xCF\x83\xC2\xB5\xCF\x84\xCE\xA6\xCE\x98\xCE\xA9\xCE\xB4\xE2\x88\x9E\xCF\x86\xCE\xB5\xE2\x88\xA9"
			"\xE2\x89\xA1\xC2\xB1\xE2\x89\xA5\xE2\x89\xA4\xE2\x8C\xA0\xE2\x8C\xA1\xC3\xB7\xE2\x89\x88\xC2\xB0\xE2\x88\x99\xC2\xB7\xE2\x88\x9A\xE2\x81\xBF\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if (codepage.Find(wxT("CP720")) != wxNOT_FOUND)
			//			newCP+=wxString( bf, wxCSConv(wxT("CP720")), 0x80);
			newCP += wxString::FromUTF8("..\xC3\xA9\xC3\xA2.\xC3\xA0.\xC3\xA7\xC3\xAA\xC3\xAB\xC3\xA8\xC3\xAF\xC3\xAE..."
			".\xD9\x91\xD9\x92\xC3\xB4\xC2\xA4\xD9\x80\xC3\xBB\xC3\xB9\xD8\xA1\xD8\xA2\xD8\xA3\xD8\xA4\xC2\xA3\xD8\xA5\xD8\xA6\xD8\xA7"
			"\xD8\xA8\xD8\xA9\xD8\xAA\xD8\xAB\xD8\xAC\xD8\xAD\xD8\xAE\xD8\xAF\xD8\xB0\xD8\xB1\xD8\xB2\xD8\xB3\xD8\xB4\xD8\xB5\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2\x95\xA1\xE2\x95\xA2\xE2\x95\x96\xE2\x95\x95\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xE2\x95\x9C\xE2\x95\x9B\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xE2\x95\x9E\xE2\x95\x9F\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x95\xA7"
			"\xE2\x95\xA8\xE2\x95\xA4\xE2\x95\xA5\xE2\x95\x99\xE2\x95\x98\xE2\x95\x92\xE2\x95\x93\xE2\x95\xAB\xE2\x95\xAA\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
			"\xD8\xB6\xD8\xB7\xD8\xB8\xD8\xB9\xD8\xBA\xD9\x81\xC2\xB5\xD9\x82\xD9\x83\xD9\x84\xD9\x85\xD9\x86\xD9\x87\xD9\x88\xD9\x89\xD9\x8A"
			"\xE2\x89\xA1\xD9\x8B\xD9\x8C\xD9\x8D\xD9\x8E\xD9\x8F\xD9\x90\xE2\x89\x88\xC2\xB0\xE2\x88\x99\xC2\xB7\xE2\x88\x9A\xE2\x81\xBF\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if (codepage.Find(wxT("CP737")) != wxNOT_FOUND)
			//			newCP+=wxString( bf, wxCSConv(wxT("CP737")), 0x80);
			newCP += wxString::FromUTF8("\xCE\x91\xCE\x92\xCE\x93\xCE\x94\xCE\x95\xCE\x96\xCE\x97\xCE\x98\xCE\x99\xCE\x9A\xCE\x9B\xCE\x9C\xCE\x9D\xCE\x9E\xCE\x9F\xCE\xA0"
			"\xCE\xA1\xCE\xA3\xCE\xA4\xCE\xA5\xCE\xA6\xCE\xA7\xCE\xA8\xCE\xA9\xCE\xB1\xCE\xB2\xCE\xB3\xCE\xB4\xCE\xB5\xCE\xB6\xCE\xB7\xCE\xB8"
			"\xCE\xB9\xCE\xBA\xCE\xBB\xCE\xBC\xCE\xBD\xCE\xBE\xCE\xBF\xCF\x80\xCF\x81\xCF\x83\xCF\x82\xCF\x84\xCF\x85\xCF\x86\xCF\x87\xCF\x88"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2\x95\xA1\xE2\x95\xA2\xE2\x95\x96\xE2\x95\x95\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xE2\x95\x9C\xE2\x95\x9B\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xE2\x95\x9E\xE2\x95\x9F\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x95\xA7"
			"\xE2\x95\xA8\xE2\x95\xA4\xE2\x95\xA5\xE2\x95\x99\xE2\x95\x98\xE2\x95\x92\xE2\x95\x93\xE2\x95\xAB\xE2\x95\xAA\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
			"\xCF\x89\xCE\xAC\xCE\xAD\xCE\xAE\xCF\x8A\xCE\xAF\xCF\x8C\xCF\x8D\xCF\x8B\xCF\x8E\xCE\x86\xCE\x88\xCE\x89\xCE\x8A\xCE\x8C\xCE\x8E"
			"\xCE\x8F\xC2\xB1\xE2\x89\xA5\xE2\x89\xA4\xCE\xAA\xCE\xAB\xC3\xB7\xE2\x89\x88\xC2\xB0\xE2\x88\x99\xC2\xB7\xE2\x88\x9A\xE2\x81\xBF\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if (codepage.Find(wxT("CP775")) != wxNOT_FOUND)
			//			newCP+=wxString( bf, wxCSConv(wxT("CP775")), 0x80);
			newCP += wxString::FromUTF8("\xC4\x86\xC3\xBC\xC3\xA9\xC4\x81\xC3\xA4\xC4\xA3\xC3\xA5\xC4\x87\xC5\x82\xC4\x93\xC5\x96\xC5\x97\xC4\xAB\xC5\xB9\xC3\x84\xC3\x85"
			"\xC3\x89\xC3\xA6\xC3\x86\xC5\x8D\xC3\xB6\xC4\xA2\xC2\xA2\xC5\x9A\xC5\x9B\xC3\x96\xC3\x9C\xC3\xB8\xC2\xA3\xC3\x98\xC3\x97\xC2\xA4"
			"\xC4\x80\xC4\xAA\xC3\xB3\xC5\xBB\xC5\xBC\xC5\xBA\xE2\x80\x9D\xC2\xA6\xC2\xA9\xC2\xAE\xC2\xAC\xC2\xBD\xC2\xBC\xC5\x81\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xC4\x84\xC4\x8C\xC4\x98\xC4\x96\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xC4\xAE\xC5\xA0\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xC5\xB2\xC5\xAA\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xC5\xBD"
			"\xC4\x85\xC4\x8D\xC4\x99\xC4\x97\xC4\xAF\xC5\xA1\xC5\xB3\xC5\xAB\xC5\xBE\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
			"\xC3\x93\xC3\x9F\xC5\x8C\xC5\x83\xC3\xB5\xC3\x95\xC2\xB5\xC5\x84\xC4\xB6\xC4\xB7\xC4\xBB\xC4\xBC\xC5\x86\xC4\x92\xC5\x85\xE2\x80\x99"
			".\xC2\xB1\xE2\x80\x9C\xC2\xBE\xC2\xB6\xC2\xA7\xC3\xB7\xE2\x80\x9E\xC2\xB0\xE2\x88\x99\xC2\xB7\xC2\xB9\xC2\xB3\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if (codepage.Find(wxT("CP850")) != wxNOT_FOUND)
			//			newCP+=wxString( bf, wxCSConv(wxT("CP850")), 0x80);
			newCP += wxString::FromUTF8("\xC3\x87\xC3\xBC\xC3\xA9\xC3\xA2\xC3\xA4\xC3\xA0\xC3\xA5\xC3\xA7\xC3\xAA\xC3\xAB\xC3\xA8\xC3\xAF\xC3\xAE\xC3\xAC\xC3\x84\xC3\x85"
			"\xC3\x89\xC3\xA6\xC3\x86\xC3\xB4\xC3\xB6\xC3\xB2\xC3\xBB\xC3\xB9\xC3\xBF\xC3\x96\xC3\x9C\xC3\xB8\xC2\xA3\xC3\x98\xC3\x97\xC6\x92"
			"\xC3\xA1\xC3\xAD\xC3\xB3\xC3\xBA\xC3\xB1\xC3\x91\xC2\xAA\xC2\xBA\xC2\xBF\xC2\xAE\xC2\xAC\xC2\xBD\xC2\xBC\xC2\xA1\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xC3\x81\xC3\x82\xC3\x80\xC2\xA9\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xC2\xA2\xC2\xA5\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xC3\xA3\xC3\x83\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xC2\xA4"
			"\xC3\xB0\xC3\x90\xC3\x8A\xC3\x8B\xC3\x88\xC4\xB1\xC3\x8D\xC3\x8E\xC3\x8F\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xC2\xA6\xC3\x8C\xE2\x96\x80"
			"\xC3\x93\xC3\x9F\xC3\x94\xC3\x92\xC3\xB5\xC3\x95\xC2\xB5\xC3\xBE\xC3\x9E\xC3\x9A\xC3\x9B\xC3\x99\xC3\xBD\xC3\x9D\xC2\xAF\xC2\xB4"
			".\xC2\xB1\xE2\x80\x97\xC2\xBE\xC2\xB6\xC2\xA7\xC3\xB7\xC2\xB8\xC2\xB0\xC2\xA8\xC2\xB7\xC2\xB9\xC2\xB3\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if (codepage.Find(wxT("CP852")) != wxNOT_FOUND)
			//			newCP+=wxString( bf, wxCSConv(wxT("CP852")), 0x80);
			newCP += wxString::FromUTF8("\xC3\x87\xC3\xBC\xC3\xA9\xC3\xA2\xC3\xA4\xC5\xAF\xC4\x87\xC3\xA7\xC5\x82\xC3\xAB\xC5\x90\xC5\x91\xC3\xAE\xC5\xB9\xC3\x84\xC4\x86"
			"\xC3\x89\xC4\xB9\xC4\xBA\xC3\xB4\xC3\xB6\xC4\xBD\xC4\xBE\xC5\x9A\xC5\x9B\xC3\x96\xC3\x9C\xC5\xA4\xC5\xA5\xC5\x81\xC3\x97\xC4\x8D"
			"\xC3\xA1\xC3\xAD\xC3\xB3\xC3\xBA\xC4\x84\xC4\x85\xC5\xBD\xC5\xBE\xC4\x98\xC4\x99\xC2\xAC\xC5\xBA\xC4\x8C\xC5\x9F\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xC3\x81\xC3\x82\xC4\x9A\xC5\x9E\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xC5\xBB\xC5\xBC\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xC4\x82\xC4\x83\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xC2\xA4"
			"\xC4\x91\xC4\x90\xC4\x8E\xC3\x8B\xC4\x8F\xC5\x87\xC3\x8D\xC3\x8E\xC4\x9B\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xC5\xA2\xC5\xAE\xE2\x96\x80"
			"\xC3\x93\xC3\x9F\xC3\x94\xC5\x83\xC5\x84\xC5\x88\xC5\xA0\xC5\xA1\xC5\x94\xC3\x9A\xC5\x95\xC5\xB0\xC3\xBD\xC3\x9D\xC5\xA3\xC2\xB4"
			".\xCB\x9D\xCB\x9B\xCB\x87\xCB\x98\xC2\xA7\xC3\xB7\xC2\xB8\xC2\xB0\xC2\xA8\xCB\x99\xC5\xB1\xC5\x98\xC5\x99\xE2\x96\xA0\xC2\xA0");

		else if (codepage.Find(wxT("CP855")) != wxNOT_FOUND)
			//			newCP+=wxString( bf, wxCSConv(wxT("CP855")), 0x80);
			newCP += wxString::FromUTF8("\xD1\x92\xD0\x82\xD1\x93\xD0\x83\xD1\x91\xD0\x81\xD1\x94\xD0\x84\xD1\x95\xD0\x85\xD1\x96\xD0\x86\xD1\x97\xD0\x87\xD1\x98\xD0\x88"
			"\xD1\x99\xD0\x89\xD1\x9A\xD0\x8A\xD1\x9B\xD0\x8B\xD1\x9C\xD0\x8C\xD1\x9E\xD0\x8E\xD1\x9F\xD0\x8F\xD1\x8E\xD0\xAE\xD1\x8A\xD0\xAA"
			"\xD0\xB0\xD0\x90\xD0\xB1\xD0\x91\xD1\x86\xD0\xA6\xD0\xB4\xD0\x94\xD0\xB5\xD0\x95\xD1\x84\xD0\xA4\xD0\xB3\xD0\x93\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xD1\x85\xD0\xA5\xD0\xB8\xD0\x98\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xD0\xB9\xD0\x99\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xD0\xBA\xD0\x9A\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xC2\xA4"
			"\xD0\xBB\xD0\x9B\xD0\xBC\xD0\x9C\xD0\xBD\xD0\x9D\xD0\xBE\xD0\x9E\xD0\xBF\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xD0\x9F\xD1\x8F\xE2\x96\x80"
			"\xD0\xAF\xD1\x80\xD0\xA0\xD1\x81\xD0\xA1\xD1\x82\xD0\xA2\xD1\x83\xD0\xA3\xD0\xB6\xD0\x96\xD0\xB2\xD0\x92\xD1\x8C\xD0\xAC\xE2\x84\x96"
			".\xD1\x8B\xD0\xAB\xD0\xB7\xD0\x97\xD1\x88\xD0\xA8\xD1\x8D\xD0\xAD\xD1\x89\xD0\xA9\xD1\x87\xD0\xA7\xC2\xA7\xE2\x96\xA0\xC2\xA0");

		else if(codepage.Find(wxT("CP856")) != wxNOT_FOUND )
			//newCP+=wxString( bf, wxCSConv(wxT("CP856")), 0x80);
			newCP += wxString::FromUTF8("\xD7\x90\xD7\x91\xD7\x92\xD7\x93\xD7\x94\xD7\x95\xD7\x96\xD7\x97\xD7\x98\xD7\x99\xD7\x9A\xD7\x9B\xD7\x9C\xD7\x9D\xD7\x9E\xD7\x9F"
			"\xD7\xA0\xD7\xA1\xD7\xA2\xD7\xA3\xD7\xA4\xD7\xA5\xD7\xA6\xD7\xA7\xD7\xA8\xD7\xA9\xD7\xAA.\xC2\xA3.\xC3\x97."
			".........\xC2\xAE\xC2\xAC\xC2\xBD\xC2\xBC.\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4...\xC2\xA9\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xC2\xA2\xC2\xA5\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC..\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xC2\xA4"
			".........\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xC2\xA6.\xE2\x96\x80"
			"......\xC2\xB5.......\xC2\xAF\xC2\xB4"
			".\xC2\xB1\xE2\x80\x97\xC2\xBE\xC2\xB6\xC2\xA7\xC3\xB7\xC2\xB8\xC2\xB0\xC2\xA8\xC2\xB7\xC2\xB9\xC2\xB3\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if(codepage.Find(wxT("CP857")) != wxNOT_FOUND ){
//			bf[0xD5-0x80]=bf[0xE7-0x80]=bf[0xF2-0x80]='.';
//			newCP+=wxString( bf, wxCSConv(wxT("CP857")), 0x80);
//			newCP[0xD5]=wxChar(0x20AC); //Euro Sign
			//Updated 0xD5 with euro sign
			newCP += wxString::FromUTF8("\xC3\x87\xC3\xBC\xC3\xA9\xC3\xA2\xC3\xA4\xC3\xA0\xC3\xA5\xC3\xA7\xC3\xAA\xC3\xAB\xC3\xA8\xC3\xAF\xC3\xAE\xC4\xB1\xC3\x84\xC3\x85"
				"\xC3\x89\xC3\xA6\xC3\x86\xC3\xB4\xC3\xB6\xC3\xB2\xC3\xBB\xC3\xB9\xC4\xB0\xC3\x96\xC3\x9C\xC3\xB8\xC2\xA3\xC3\x98\xC5\x9E\xC5\x9F"
				"\xC3\xA1\xC3\xAD\xC3\xB3\xC3\xBA\xC3\xB1\xC3\x91\xC4\x9E\xC4\x9F\xC2\xBF\xC2\xAE\xC2\xAC\xC2\xBD\xC2\xBC\xC2\xA1\xC2\xAB\xC2\xBB"
				"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xC3\x81\xC3\x82\xC3\x80\xC2\xA9\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xC2\xA2\xC2\xA5\xE2\x94\x90"
				"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xC3\xA3\xC3\x83\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xC2\xA4"
				"\xC2\xBA\xC2\xAA\xC3\x8A\xC3\x8B\xC3\x88\xE2\x82\xAC\xC3\x8D\xC3\x8E\xC3\x8F\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xC2\xA6\xC3\x8C\xE2\x96\x80"
				"\xC3\x93\xC3\x9F\xC3\x94\xC3\x92\xC3\xB5\xC3\x95\xC2\xB5.\xC3\x97\xC3\x9A\xC3\x9B\xC3\x99\xC3\xAC\xC3\xBF\xC2\xAF\xC2\xB4"
				".\xC2\xB1.\xC2\xBE\xC2\xB6\xC2\xA7\xC3\xB7\xC2\xB8\xC2\xB0\xC2\xA8\xC2\xB7\xC2\xB9\xC2\xB3\xC2\xB2\xE2\x96\xA0\xC2\xA0");
		}

		else if(codepage.Find(wxT("CP858")) != wxNOT_FOUND ){
			newCP=PrepareCodepageTable(wxT("PC/DOS CP850"));
			newCP[0xD5]=wxChar(0x20AC);
			}

		else if(codepage.Find(wxT("CP860")) != wxNOT_FOUND )
			//			newCP+=wxString( bf, wxCSConv(wxT("CP860")), 0x80);
			newCP += wxString::FromUTF8("\xC3\x87\xC3\xBC\xC3\xA9\xC3\xA2\xC3\xA3\xC3\xA0\xC3\x81\xC3\xA7\xC3\xAA\xC3\x8A\xC3\xA8\xC3\x8D\xC3\x94\xC3\xAC\xC3\x83\xC3\x82"
			"\xC3\x89\xC3\x80\xC3\x88\xC3\xB4\xC3\xB5\xC3\xB2\xC3\x9A\xC3\xB9\xC3\x8C\xC3\x95\xC3\x9C\xC2\xA2\xC2\xA3\xC3\x99\xE2\x82\xA7\xC3\x93"
			"\xC3\xA1\xC3\xAD\xC3\xB3\xC3\xBA\xC3\xB1\xC3\x91\xC2\xAA\xC2\xBA\xC2\xBF\xC3\x92\xC2\xAC\xC2\xBD\xC2\xBC\xC2\xA1\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2\x95\xA1\xE2\x95\xA2\xE2\x95\x96\xE2\x95\x95\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xE2\x95\x9C\xE2\x95\x9B\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xE2\x95\x9E\xE2\x95\x9F\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x95\xA7"
			"\xE2\x95\xA8\xE2\x95\xA4\xE2\x95\xA5\xE2\x95\x99\xE2\x95\x98\xE2\x95\x92\xE2\x95\x93\xE2\x95\xAB\xE2\x95\xAA\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
			"\xCE\xB1\xC3\x9F\xCE\x93\xCF\x80\xCE\xA3\xCF\x83\xC2\xB5\xCF\x84\xCE\xA6\xCE\x98\xCE\xA9\xCE\xB4\xE2\x88\x9E\xCF\x86\xCE\xB5\xE2\x88\xA9"
			"\xE2\x89\xA1\xC2\xB1\xE2\x89\xA5\xE2\x89\xA4\xE2\x8C\xA0\xE2\x8C\xA1\xC3\xB7\xE2\x89\x88\xC2\xB0\xE2\x88\x99\xC2\xB7\xE2\x88\x9A\xE2\x81\xBF\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if(codepage.Find(wxT("CP861")) != wxNOT_FOUND )
			//			newCP+=wxString( bf, wxCSConv(wxString::FromUTF8("CP861")), 0x80);
			newCP += wxString::FromUTF8("\xC3\x87\xC3\xBC\xC3\xA9\xC3\xA2\xC3\xA4\xC3\xA0\xC3\xA5\xC3\xA7\xC3\xAA\xC3\xAB\xC3\xA8\xC3\x90\xC3\xB0\xC3\x9E\xC3\x84\xC3\x85"
			"\xC3\x89\xC3\xA6\xC3\x86\xC3\xB4\xC3\xB6\xC3\xBE\xC3\xBB\xC3\x9D\xC3\xBD\xC3\x96\xC3\x9C\xC3\xB8\xC2\xA3\xC3\x98\xE2\x82\xA7\xC6\x92"
			"\xC3\xA1\xC3\xAD\xC3\xB3\xC3\xBA\xC3\x81\xC3\x8D\xC3\x93\xC3\x9A\xC2\xBF\xE2\x8C\x90\xC2\xAC\xC2\xBD\xC2\xBC\xC2\xA1\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2\x95\xA1\xE2\x95\xA2\xE2\x95\x96\xE2\x95\x95\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xE2\x95\x9C\xE2\x95\x9B\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xE2\x95\x9E\xE2\x95\x9F\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x95\xA7"
			"\xE2\x95\xA8\xE2\x95\xA4\xE2\x95\xA5\xE2\x95\x99\xE2\x95\x98\xE2\x95\x92\xE2\x95\x93\xE2\x95\xAB\xE2\x95\xAA\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
			"\xCE\xB1\xC3\x9F\xCE\x93\xCF\x80\xCE\xA3\xCF\x83\xC2\xB5\xCF\x84\xCE\xA6\xCE\x98\xCE\xA9\xCE\xB4\xE2\x88\x9E\xCF\x86\xCE\xB5\xE2\x88\xA9"
			"\xE2\x89\xA1\xC2\xB1\xE2\x89\xA5\xE2\x89\xA4\xE2\x8C\xA0\xE2\x8C\xA1\xC3\xB7\xE2\x89\x88\xC2\xB0\xE2\x88\x99\xC2\xB7\xE2\x88\x9A\xE2\x81\xBF\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if(codepage.Find(wxT("CP862")) != wxNOT_FOUND )
			//			newCP+=wxString( bf, wxCSConv(wxString::FromUTF8("CP862")), 0x80);
			newCP += wxString::FromUTF8("\xD7\x90\xD7\x91\xD7\x92\xD7\x93\xD7\x94\xD7\x95\xD7\x96\xD7\x97\xD7\x98\xD7\x99\xD7\x9A\xD7\x9B\xD7\x9C\xD7\x9D\xD7\x9E\xD7\x9F"
			"\xD7\xA0\xD7\xA1\xD7\xA2\xD7\xA3\xD7\xA4\xD7\xA5\xD7\xA6\xD7\xA7\xD7\xA8\xD7\xA9\xD7\xAA\xC2\xA2\xC2\xA3\xC2\xA5\xE2\x82\xA7\xC6\x92"
			"\xC3\xA1\xC3\xAD\xC3\xB3\xC3\xBA\xC3\xB1\xC3\x91\xC2\xAA\xC2\xBA\xC2\xBF\xE2\x8C\x90\xC2\xAC\xC2\xBD\xC2\xBC\xC2\xA1\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2\x95\xA1\xE2\x95\xA2\xE2\x95\x96\xE2\x95\x95\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xE2\x95\x9C\xE2\x95\x9B\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xE2\x95\x9E\xE2\x95\x9F\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x95\xA7"
			"\xE2\x95\xA8\xE2\x95\xA4\xE2\x95\xA5\xE2\x95\x99\xE2\x95\x98\xE2\x95\x92\xE2\x95\x93\xE2\x95\xAB\xE2\x95\xAA\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
			"\xCE\xB1\xC3\x9F\xCE\x93\xCF\x80\xCE\xA3\xCF\x83\xC2\xB5\xCF\x84\xCE\xA6\xCE\x98\xCE\xA9\xCE\xB4\xE2\x88\x9E\xCF\x86\xCE\xB5\xE2\x88\xA9"
			"\xE2\x89\xA1\xC2\xB1\xE2\x89\xA5\xE2\x89\xA4\xE2\x8C\xA0\xE2\x8C\xA1\xC3\xB7\xE2\x89\x88\xC2\xB0\xE2\x88\x99\xC2\xB7\xE2\x88\x9A\xE2\x81\xBF\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if(codepage.Find(wxT("CP863")) != wxNOT_FOUND )
			//			newCP+=wxString( bf, wxCSConv(wxString::FromUTF8("CP863")), 0x80);
			newCP += wxString::FromUTF8("\xC3\x87\xC3\xBC\xC3\xA9\xC3\xA2\xC3\x82\xC3\xA0\xC2\xB6\xC3\xA7\xC3\xAA\xC3\xAB\xC3\xA8\xC3\xAF\xC3\xAE\xE2\x80\x97\xC3\x80\xC2\xA7"
			"\xC3\x89\xC3\x88\xC3\x8A\xC3\xB4\xC3\x8B\xC3\x8F\xC3\xBB\xC3\xB9\xC2\xA4\xC3\x94\xC3\x9C\xC2\xA2\xC2\xA3\xC3\x99\xC3\x9B\xC6\x92"
			"\xC2\xA6\xC2\xB4\xC3\xB3\xC3\xBA\xC2\xA8\xC2\xB8\xC2\xB3\xC2\xAF\xC3\x8E\xE2\x8C\x90\xC2\xAC\xC2\xBD\xC2\xBC\xC2\xBE\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2\x95\xA1\xE2\x95\xA2\xE2\x95\x96\xE2\x95\x95\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xE2\x95\x9C\xE2\x95\x9B\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xE2\x95\x9E\xE2\x95\x9F\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x95\xA7"
			"\xE2\x95\xA8\xE2\x95\xA4\xE2\x95\xA5\xE2\x95\x99\xE2\x95\x98\xE2\x95\x92\xE2\x95\x93\xE2\x95\xAB\xE2\x95\xAA\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
			"\xCE\xB1\xC3\x9F\xCE\x93\xCF\x80\xCE\xA3\xCF\x83\xC2\xB5\xCF\x84\xCE\xA6\xCE\x98\xCE\xA9\xCE\xB4\xE2\x88\x9E\xCF\x86\xCE\xB5\xE2\x88\xA9"
			"\xE2\x89\xA1\xC2\xB1\xE2\x89\xA5\xE2\x89\xA4\xE2\x8C\xA0\xE2\x8C\xA1\xC3\xB7\xE2\x89\x88\xC2\xB0\xE2\x88\x99\xC2\xB7\xE2\x88\x9A\xE2\x81\xBF\xC2\xB2\xE2\x96\xA0\xC2\xA0");

		else if(codepage.Find(wxT("CP864")) != wxNOT_FOUND ){
			//0xA7 replaced with EUR
			//			newCP+=wxString( bf, wxCSConv(wxString::FromUTF8("CP864")), 0x80);
			newCP += wxString::FromUTF8("\xC2\xB0\xC2\xB7\xE2\x88\x99\xE2\x88\x9A\xE2\x96\x92\xE2\x94\x80\xE2\x94\x82\xE2\x94\xBC\xE2\x94\xA4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\xB4\xE2\x94\x90\xE2\x94\x8C\xE2\x94\x94\xE2\x94\x98"
				"\xCE\xB2\xE2\x88\x9E\xCF\x86\xC2\xB1\xC2\xBD\xC2\xBC\xE2\x89\x88\xC2\xAB\xC2\xBB\xEF\xBB\xB7\xEF\xBB\xB8..\xEF\xBB\xBB\xEF\xBB\xBC."
				"\xC2\xA0.\xEF\xBA\x82\xC2\xA3\xC2\xA4\xEF\xBA\x84.\xE2\x82\xAC\xEF\xBA\x8E\xEF\xBA\x8F\xEF\xBA\x95\xEF\xBA\x99\xD8\x8C\xEF\xBA\x9D\xEF\xBA\xA1\xEF\xBA\xA5"
				"\xD9\xA0\xD9\xA1\xD9\xA2\xD9\xA3\xD9\xA4\xD9\xA5\xD9\xA6\xD9\xA7\xD9\xA8\xD9\xA9\xEF\xBB\x91\xD8\x9B\xEF\xBA\xB1\xEF\xBA\xB5\xEF\xBA\xB9\xD8\x9F"
				"\xC2\xA2\xEF\xBA\x80\xEF\xBA\x81\xEF\xBA\x83\xEF\xBA\x85\xEF\xBB\x8A\xEF\xBA\x8B\xEF\xBA\x8D\xEF\xBA\x91\xEF\xBA\x93\xEF\xBA\x97\xEF\xBA\x9B\xEF\xBA\x9F\xEF\xBA\xA3\xEF\xBA\xA7\xEF\xBA\xA9"
				"\xEF\xBA\xAB\xEF\xBA\xAD\xEF\xBA\xAF\xEF\xBA\xB3\xEF\xBA\xB7\xEF\xBA\xBB\xEF\xBA\xBF\xEF\xBB\x81\xEF\xBB\x85\xEF\xBB\x8B\xEF\xBB\x8F\xC2\xA6\xC2\xAC\xC3\xB7\xC3\x97\xEF\xBB\x89"
				"\xD9\x80\xEF\xBB\x93\xEF\xBB\x97\xEF\xBB\x9B\xEF\xBB\x9F\xEF\xBB\xA3\xEF\xBB\xA7\xEF\xBB\xAB\xEF\xBB\xAD\xEF\xBB\xAF\xEF\xBB\xB3\xEF\xBA\xBD\xEF\xBB\x8C\xEF\xBB\x8E\xEF\xBB\x8D\xEF\xBB\xA1"
				"\xEF\xB9\xBD\xD9\x91\xEF\xBB\xA5\xEF\xBB\xA9\xEF\xBB\xAC\xEF\xBB\xB0\xEF\xBB\xB2\xEF\xBB\x90\xEF\xBB\x95\xEF\xBB\xB5\xEF\xBB\xB6\xEF\xBB\x9D\xEF\xBB\x99\xEF\xBB\xB1\xE2\x96\xA0.");
			newCP[0x25] = wxChar(0x066A); // ARABIC PERCENT SIGN ⟨٪⟩
			}

		else if(codepage.Find(wxT("CP865")) != wxNOT_FOUND ){
			newCP=PrepareCodepageTable(wxT("PC/DOS CP437" ));
			newCP[0x9B]=wxChar(0xF8);
			newCP[0x9D]=wxChar(0xD8);
			newCP[0xAF]=wxChar(0xA4);
			}

		else if(codepage.Find(wxT("CP866")) != wxNOT_FOUND )
			//			newCP+=wxString( bf, wxCSConv(wxString::FromUTF8("CP866")), 0x80);
			newCP += wxString::FromUTF8("\xD0\x90\xD0\x91\xD0\x92\xD0\x93\xD0\x94\xD0\x95\xD0\x96\xD0\x97\xD0\x98\xD0\x99\xD0\x9A\xD0\x9B\xD0\x9C\xD0\x9D\xD0\x9E\xD0\x9F"
			"\xD0\xA0\xD0\xA1\xD0\xA2\xD0\xA3\xD0\xA4\xD0\xA5\xD0\xA6\xD0\xA7\xD0\xA8\xD0\xA9\xD0\xAA\xD0\xAB\xD0\xAC\xD0\xAD\xD0\xAE\xD0\xAF"
			"\xD0\xB0\xD0\xB1\xD0\xB2\xD0\xB3\xD0\xB4\xD0\xB5\xD0\xB6\xD0\xB7\xD0\xB8\xD0\xB9\xD0\xBA\xD0\xBB\xD0\xBC\xD0\xBD\xD0\xBE\xD0\xBF"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2\x95\xA1\xE2\x95\xA2\xE2\x95\x96\xE2\x95\x95\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xE2\x95\x9C\xE2\x95\x9B\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xE2\x95\x9E\xE2\x95\x9F\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x95\xA7"
			"\xE2\x95\xA8\xE2\x95\xA4\xE2\x95\xA5\xE2\x95\x99\xE2\x95\x98\xE2\x95\x92\xE2\x95\x93\xE2\x95\xAB\xE2\x95\xAA\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
			"\xD1\x80\xD1\x81\xD1\x82\xD1\x83\xD1\x84\xD1\x85\xD1\x86\xD1\x87\xD1\x88\xD1\x89\xD1\x8A\xD1\x8B\xD1\x8C\xD1\x8D\xD1\x8E\xD1\x8F"
			"\xD0\x81\xD1\x91\xD0\x84\xD1\x94\xD0\x87\xD1\x97\xD0\x8E\xD1\x9E\xC2\xB0\xE2\x88\x99\xC2\xB7\xE2\x88\x9A\xE2\x84\x96\xC2\xA4\xE2\x96\xA0\xC2\xA0");

		else if (codepage.Find(wxT("CP869")) != wxNOT_FOUND)
			//			newCP+=wxString( bf, wxCSConv(wxString::FromUTF8("CP869")), 0x80);
			newCP += wxString::FromUTF8("......\xCE\x86.\xC2\xB7\xC2\xAC\xC2\xA6\xE2\x80\x98\xE2\x80\x99\xCE\x88\xE2\x80\x95\xCE\x89"
			"\xCE\x8A\xCE\xAA\xCE\x8C..\xCE\x8E\xCE\xAB\xC2\xA9\xCE\x8F\xC2\xB2\xC2\xB3\xCE\xAC\xC2\xA3\xCE\xAD\xCE\xAE\xCE\xAF"
			"\xCF\x8A\xCE\x90\xCF\x8C\xCF\x8D\xCE\x91\xCE\x92\xCE\x93\xCE\x94\xCE\x95\xCE\x96\xCE\x97\xC2\xBD\xCE\x98\xCE\x99\xC2\xAB\xC2\xBB"
			"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xCE\x9A\xCE\x9B\xCE\x9C\xCE\x9D\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x97\xE2\x95\x9D\xCE\x9E\xCE\x9F\xE2\x94\x90"
			"\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xCE\xA0\xCE\xA1\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xCE\xA3"
			"\xCE\xA4\xCE\xA5\xCE\xA6\xCE\xA7\xCE\xA8\xCE\xA9\xCE\xB1\xCE\xB2\xCE\xB3\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xCE\xB4\xCE\xB5\xE2\x96\x80"
			"\xCE\xB6\xCE\xB7\xCE\xB8\xCE\xB9\xCE\xBA\xCE\xBB\xCE\xBC\xCE\xBD\xCE\xBE\xCE\xBF\xCF\x80\xCF\x81\xCF\x83\xCF\x82\xCF\x84\xCE\x84"
			".\xC2\xB1\xCF\x85\xCF\x86\xCF\x87\xC2\xA7\xCF\x88\xCE\x85\xC2\xB0\xC2\xA8\xCF\x89\xCF\x8B\xCE\xB0\xCF\x8E\xE2\x96\xA0\xC2\xA0");

		else if (codepage.Find(wxT("CP1006")) != wxNOT_FOUND)
			//			newCP+=wxString( bf, wxCSConv(wxString::FromUTF8("CP1006")), 0x80);
			newCP += wxString::FromUTF8("\xC2\xA0\xDB\xB0\xDB\xB1\xDB\xB2\xDB\xB3\xDB\xB4\xDB\xB5\xDB\xB6\xDB\xB7\xDB\xB8\xDB\xB9\xD8\x8C\xD8\x9B.\xD8\x9F\xEF\xBA\x81"
			"\xEF\xBA\x8D\xEF\xBA\x8E\xEF\xBA\x8E\xEF\xBA\x8F\xEF\xBA\x91\xEF\xAD\x96\xEF\xAD\x98\xEF\xBA\x93\xEF\xBA\x95\xEF\xBA\x97\xEF\xAD\xA6\xEF\xAD\xA8\xEF\xBA\x99\xEF\xBA\x9B\xEF\xBA\x9D\xEF\xBA\x9F"
			"\xEF\xAD\xBA\xEF\xAD\xBC\xEF\xBA\xA1\xEF\xBA\xA3\xEF\xBA\xA5\xEF\xBA\xA7\xEF\xBA\xA9\xEF\xAE\x84\xEF\xBA\xAB\xEF\xBA\xAD\xEF\xAE\x8C\xEF\xBA\xAF\xEF\xAE\x8A\xEF\xBA\xB1\xEF\xBA\xB3\xEF\xBA\xB5"
			"\xEF\xBA\xB7\xEF\xBA\xB9\xEF\xBA\xBB\xEF\xBA\xBD\xEF\xBA\xBF\xEF\xBB\x81\xEF\xBB\x85\xEF\xBB\x89\xEF\xBB\x8A\xEF\xBB\x8B\xEF\xBB\x8C\xEF\xBB\x8D\xEF\xBB\x8E\xEF\xBB\x8F\xEF\xBB\x90\xEF\xBB\x91"
			"\xEF\xBB\x93\xEF\xBB\x95\xEF\xBB\x97\xEF\xBB\x99\xEF\xBB\x9B\xEF\xAE\x92\xEF\xAE\x94\xEF\xBB\x9D\xEF\xBB\x9F\xEF\xBB\xA0\xEF\xBB\xA1\xEF\xBB\xA3\xEF\xAE\x9E\xEF\xBB\xA5\xEF\xBB\xA7\xEF\xBA\x85"
			"\xEF\xBB\xAD\xEF\xAE\xA6\xEF\xAE\xA8\xEF\xAE\xA9\xEF\xAE\xAA\xEF\xBA\x80\xEF\xBA\x89\xEF\xBA\x8A\xEF\xBA\x8B\xEF\xBB\xB1\xEF\xBB\xB2\xEF\xBB\xB3\xEF\xAE\xB0\xEF\xAE\xAE\xEF\xB9\xBC\xEF\xB9\xBD");

		else if (codepage.Find(wxT("KZ-1048")) != wxNOT_FOUND){
			newCP += wxString::FromUTF8("\xD0\x82\xD0\x83\xE2\x80\x9A\xD1\x93\xE2\x80\x9E\xE2\x80\xA6\xE2\x80\xA0\xE2\x80\xA1\xE2\x82\xAC\xE2\x80\xB0\xD0\x89\xE2\x80\xB9\xD0\x8A\xD2\x9A\xD2\xBA\xD0\x8F"
				"\xD1\x92\xE2\x80\x98\xE2\x80\x99\xE2\x80\x9C\xE2\x80\x9D\xE2\x80\xA2\xE2\x80\x93\xE2\x80\x94.\xE2\x84\xA2\xD1\x99\xE2\x80\xBA\xD1\x9A\xD2\x9B\xD2\xBB\xD1\x9F"
				"\xC2\xA0\xD2\xB0\xD2\xB1\xD3\x98\xC2\xA4\xD3\xA8\xC2\xA6\xC2\xA7\xD0\x81\xC2\xA9\xD2\x92\xC2\xAB\xC2\xAC.\xC2\xAE\xD2\xAE"
				"\xC2\xB0\xC2\xB1\xD0\x86\xD1\x96\xD3\xA9\xC2\xB5\xC2\xB6\xC2\xB7\xD1\x91\xE2\x84\x96\xD2\x93\xC2\xBB\xD3\x99\xD2\xA2\xD2\xA3\xD2\xAF");
			for (int i = 0; i<0x40; i++)
				newCP+=wxChar(0x0410+i);
            /*
							"\x0410\x0411\x0412\x0413\x0414\x0415\x0416\x0417\x0418\x0419\x041A\x041B\x041C\x041D\x041E\x041F"\
							"\x0420\x0421\x0422\x0423\x0424\x0425\x0426\x0427\x0428\x0429\x042A\x042B\x042C\x042D\x042E\x042F"\
							"\x0430\x0431\x0432\x0433\x0434\x0435\x0436\x0437\x0438\x0439\x043A\x043B\x043C\x043D\x043E\x043F"\
							"\x0440\x0441\x0442\x0443\x0444\x0445\x0446\x0447\x0448\x0449\x044A\x044B\x044C\x044D\x044E\x044F" );
            */
			}
		else if(codepage.Find(wxT("MIK")) != wxNOT_FOUND ){
			for (unsigned i=0x80; i<0xC0 ; i++)
				newCP += wxChar(i-0x80+0x0410);

			newCP += wxString::FromUTF8("\xE2\x94\x94\xE2\x94\xB4\xE2\x94\xAC\xE2\x94\x9C\xE2\x94\x80\xE2\x94\xBC\xE2\x95\xA3\xE2\x95\x91\xE2\x95\x9A\xE2\x95\x94\xE2\x95\xA9\xE2\x95\xA6\xE2\x95\xA0\xE2\x95\x90\xE2\x95\xAC\xE2\x94\x90"
				"\xE2\x96\x91\xE2\x96\x92\xE2\x96\x93\xE2\x94\x82\xE2\x94\xA4\xE2\x84\x96\xC2\xA7\xE2\x95\x97\xE2\x95\x9D\xE2\x94\x98\xE2\x94\x8C\xE2\x96\x88\xE2\x96\x84\xE2\x96\x8C\xE2\x96\x90\xE2\x96\x80"
				"\xCE\xB1\xC3\x9F\xCE\x93\xCF\x80\xCE\xA3\xCF\x83\xC2\xB5\xCF\x84\xCE\xA6\xCE\x98\xCE\xA9\xCE\xB4\xE2\x88\x9E\xCF\x86\xCE\xB5\xE2\x88\xA9"
				"\xE2\x89\xA1\xC2\xB1\xE2\x89\xA5\xE2\x89\xA4\xE2\x8C\xA0\xE2\x8C\xA1\xC3\xB7\xE2\x89\x88\xC2\xB0\xE2\x88\x99\xC2\xB7\xE2\x88\x9A\xE2\x81\xBF\xC2\xB2\xE2\x96\xA0\xC2\xA0");
		}

		else if(codepage.Find(wxT("Kamenick")) != wxNOT_FOUND ){
			newCP=PrepareCodepageTable(wxT("PC/DOS CP437" ));
			newCP = newCP.Mid(0, 0x80)
				+ wxString::FromUTF8("\xC4\x8C\xC3\xBC\xC3\xA9\xC4\x8F\xC3\xA4\xC4\x8E\xC5\xA4\xC4\x8D\xC4\x9B\xC4\x9A\xC4\xB9\xC3\x8D\xC4\xBE\xC4\xBA\xC3\x84\xC3\x81"
				"\xC3\x89\xC5\xBE\xC5\xBD\xC3\xB4\xC3\xB6\xC3\x93\xC5\xAF\xC3\x9A\xC3\xBD\xC3\x96\xC3\x9C\xC5\xA0\xC4\xBD\xC3\x9D\xC5\x98\xC5\xA5"
				"\xC3\xA1\xC3\xAD\xC3\xB3\xC3\xBA\xC5\x88\xC5\x87\xC5\xAE\xC3\x94\xC5\xA1\xC5\x99\xC5\x95\xC5\x94\xC2\xBC\xC2\xA7\xC2\xAB\xC2\xBB")
				+ newCP.Mid(0xB0, 0xFF - 0xB0);
		}

		else if(codepage.Find(wxT("Mazovia")) != wxNOT_FOUND ){
			newCP=PrepareCodepageTable(wxT("PC/DOS CP437" ));
			newCP = newCP.Mid(0, 0x80)
				+ wxString::FromUTF8("\xC3\x87\xC3\xBC\xC3\xA9\xC3\xA2\xC3\xA4\xC3\xA0\xC4\x85\xC3\xA7\xC3\xAA\xC3\xAB\xC3\xA8\xC3\xAF\xC3\xAE\xC4\x87\xC3\x84\xC4\x84"
				"\xC4\x98\xC4\x99\xC5\x82\xC3\xB4\xC3\xB6\xC4\x86\xC3\xBB\xC3\xB9\xC5\x9A\xC3\x96\xC3\x9C\xC2\xA2\xC5\x81\xC2\xA5\xC5\x9B\xC6\x92"
				"\xC5\xB9\xC5\xBB\xC3\xB3\xC3\x93\xC5\x84\xC5\x83\xC5\xBA\xC5\xBC\xC2\xBF\xE2\x8C\x90\xC2\xAC\xC2\xBD\xC2\xBC\xC2\xA1\xC2\xAB\xC2\xBB")
				+ newCP.Mid(0xB0, 0xFF - 0xB0);
		}
		else if(codepage.Find(wxT("Iran")) != wxNOT_FOUND ){
			newCP=PrepareCodepageTable(wxT("PC/DOS CP437" ));
			newCP = newCP.Mid(0, 0x80)
				+ wxString::FromUTF8("\xDB\xB0\xDB\xB1\xDB\xB2\xDB\xB3\xDB\xB4\xDB\xB5\xDB\xB6\xDB\xB7\xDB\xB8\xDB\xB9\xD8\x8C\xD9\x80\xD8\x9F\xEF\xBA\x81\xEF\xBA\x8B\xD8\xA1"
				"\xEF\xBA\x8D\xEF\xBA\x8E\xEF\xBA\x8F\xEF\xBA\x91\xEF\xAD\x96\xEF\xAD\x98\xEF\xBA\x95\xEF\xBA\x97\xEF\xBA\x99\xEF\xBA\x9B\xEF\xBA\x9D\xEF\xBA\x9F\xEF\xAD\xBC\xEF\xAD\xBC\xEF\xBA\xA1\xEF\xBA\xA3"
				"\xEF\xBA\xA5\xEF\xBA\xA7\xD8\xAF\xD8\xB0\xD8\xB1\xD8\xB2\xDA\x98\xEF\xBA\xB1\xEF\xBA\xB3\xEF\xBA\xB5\xEF\xBA\xB7\xEF\xBA\xB9\xEF\xBA\xBB\xEF\xBA\xBD\xEF\xBA\xBF\xD8\xB7")
				+ newCP.Mid(0xB0, 0xFF - 0xB0);
		}

		}

	else	if(codepage.Find(wxT("ISO/IEC")) != wxNOT_FOUND ){
		if(codepage.Find(wxT("6937")) != wxNOT_FOUND){
			newCP=PrepareCodepageTable(wxT("ASCII")).Mid(0,0xA0);
			//SHY replaced with dot (at 0xFF)
			newCP += wxString::FromUTF8("\xC2\xA0\xC2\xA1\xC2\xA2\xC2\xA3.\xC2\xA5.\xC2\xA7\xC2\xA4\xE2\x80\x98\xE2\x80\x9C\xC2\xAB\xE2\x86\x90\xE2\x86\x91\xE2\x86\x92\xE2\x86\x93"
				"\xC2\xB0\xC2\xB1\xC2\xB2\xC2\xB3\xC3\x97\xC2\xB5\xC2\xB6\xC2\xB7\xC3\xB7\xE2\x80\x99\xE2\x80\x9D\xC2\xBB\xC2\xBC\xC2\xBD\xC2\xBE\xC2\xBF.\xCC\x80\xCC\x81"
				"\xCC\x82\xCC\x83\xCC\x84\xCC\x86\xCC\x87\xCC\x88.\xCC\x8A\xCC\xA7.\xCC\x8B\xCC\xA8\xCC\x8C\xE2\x80\x95\xC2\xB9"
				"\xC2\xAE\xC2\xA9\xE2\x84\xA2\xE2\x99\xAA\xC2\xAC\xC2\xA6....\xE2\x85\x9B\xE2\x85\x9C\xE2\x85\x9D\xE2\x85\x9E\xE2\x84\xA6\xC3\x86\xC4\x90\xC2\xAA"
				"\xC4\xA6.\xC4\xB2\xC4\xBF\xC5\x81\xC3\x98\xC5\x92\xC2\xBA\xC3\x9E\xC5\xA6\xC5\x8A\xC5\x89\xC4\xB8\xC3\xA6\xC4\x91"
				"\xC3\xB0\xC4\xA7\xC4\xB1\xC4\xB3\xC5\x80\xC5\x82\xC3\xB8\xC5\x93\xC3\x9F\xC3\xBE\xC5\xA7\xC5\x8B.");
			return CodepageTable = newCP;
			}


		//Masking default area
		for (unsigned i=0; i<=0xFF ; i++)
			bf[i] =  (i< 0x20 || i==0x7F || i==0xAD || (i>=0x80 && i<=0x9F ))	? '.' : i;

		//Detecting exact encoding
		int q=codepage.Find(wxT("8859-"))+5;

		//Filtering gaps
		if(codepage.Mid(q,2).StartsWith(wxT("3 ")))			bf[0xA5]=bf[0xAE]=bf[0xBE]=bf[0xC3]=bf[0xD0]=bf[0xE3]=bf[0xF0]='.';
		else if(codepage.Mid(q,2).StartsWith(wxT("6 "))){	//Arabic
			for(int i=0xA1 ; i<=0xC0 ; i++) bf[i]='.';
			bf[0xA4]=0xA4;
			bf[0xAC]=0xAC;
			bf[0xBB]=0xBB;
			bf[0xBF]=0xBF;
			for(int i=0xDB ; i<=0xDF ; i++) bf[i]='.';
			for(int i=0xF3 ; i<=0xFF ; i++) bf[i]='.';
			}
		else if(codepage.Mid(q,2).StartsWith(wxT("7 ")))	bf[0xAE]=bf[0xD2]=bf[0xFF]='.';
		else if(codepage.Mid(q,2).StartsWith(wxT("8 "))){	//Hebrew
			for(int i=0xBF ; i<=0xDE ; i++) bf[i]='.';
			for(int i=0xFB ; i<=0xFF ; i++) bf[i]='.';
			bf[0xA1]='.';
			}
		else if(codepage.Mid(q,2).StartsWith(wxT("11")))	bf[0xDB]=bf[0xDC]=bf[0xDD]=bf[0xDE]=bf[0xFC]=bf[0xFD]=bf[0xFE]=bf[0xFF]='.';

		//Encoding
		if		 (codepage.Mid(q,2).StartsWith(wxT("1 ")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_1), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("2 ")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_2), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("3 ")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_3), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("4 ")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_4), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("5 ")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_5), 256);
		// Arabic Output not looks good.
		else if(codepage.Mid(q,2).StartsWith(wxT("6 ")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_6), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("7 ")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_7), 256);
		// Hebrew Output not looks good.
		else if(codepage.Mid(q,2).StartsWith(wxT("8 ")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_8), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("9 ")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_9), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("10")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_10), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("11")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_11), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("12")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_12), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("13")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_13), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("14")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_14), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("15")))	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_ISO8859_15), 256);
		else if(codepage.Mid(q,2).StartsWith(wxT("16"))){
			//newCP+=wxString( bf, wxCSConv(wxT("ISO8859-16")), 256);
			newCP=PrepareCodepageTable(wxT("ASCII")).Mid(0,0xA0);
			newCP += wxString::FromUTF8("\xC2\xA0\xC4\x84\xC4\x85\xC5\x81\xE2\x82\xAC\xE2\x80\x9E\xC5\xA0\xC2\xA7\xC5\xA1\xC2\xA9\xC8\x98\xC2\xAB\xC5\xB9\xC2\xAD\xC5\xBA\xC5\xBB"
				"\xC2\xB0\xC2\xB1\xC4\x8C\xC5\x82\xC5\xBD\xE2\x80\x9D\xC2\xB6\xC2\xB7\xC5\xBE\xC4\x8D\xC8\x99\xC2\xBB\xC5\x92\xC5\x93\xC5\xB8\xC5\xBC"
				"\xC3\x80\xC3\x81\xC3\x82\xC4\x82\xC3\x84\xC4\x86\xC3\x86\xC3\x87\xC3\x88\xC3\x89\xC3\x8A\xC3\x8B\xC3\x8C\xC3\x8D\xC3\x8E\xC3\x8F"
				"\xC4\x90\xC5\x83\xC3\x92\xC3\x93\xC3\x94\xC5\x90\xC3\x96\xC5\x9A\xC5\xB0\xC3\x99\xC3\x9A\xC3\x9B\xC3\x9C\xC4\x98\xC8\x9A\xC3\x9F"
				"\xC3\xA0\xC3\xA1\xC3\xA2\xC4\x83\xC3\xA4\xC4\x87\xC3\xA6\xC3\xA7\xC3\xA8\xC3\xA9\xC3\xAA\xC3\xAB\xC3\xAC\xC3\xAD\xC3\xAE\xC3\xAF"
				"\xC4\x91\xC5\x84\xC3\xB2\xC3\xB3\xC3\xB4\xC5\x91\xC3\xB6\xC5\x9B\xC5\xB1\xC3\xB9\xC3\xBA\xC3\xBB\xC3\xBC\xC4\x99\xC8\x9B\xC3\xBF");
		}
		}

	// Windows Code Pages
	else if(codepage.Find(wxT("Windows")) != wxNOT_FOUND ){
		if(codepage.Find(wxT("CP874")) != wxNOT_FOUND ){
			newCP=PrepareCodepageTable(wxT("ISO/IEC 8859-11"));
			unsigned char a[]="\x80\xB5\x91\x92\x93\x94\x95\x96\x97";  //Patch Index
			wxString b = wxString::FromUTF8("\xE2\x82\xAC\xE2\x80\xA6\xE2\x80\x98\xE2\x80\x99\xE2\x80\x9C\xE2\x80\x9D\xE2\x80\xA2\xE2\x80\x93\xE2\x80\x94");	//Patch Value
			for (int i = 0; i<9; i++)
				newCP[a[i]]=b[i];
			}
		else if(codepage.Find(wxT("CP932")) != wxNOT_FOUND ) FontEnc=wxFONTENCODING_CP932;//ShiftJS
		else if(codepage.Find(wxT("CP936")) != wxNOT_FOUND ) FontEnc=wxFONTENCODING_CP936;//GBK
		else if(codepage.Find(wxT("CP949")) != wxNOT_FOUND ) FontEnc=wxFONTENCODING_CP949; //EUC-KR
		else if(codepage.Find(wxT("CP950")) != wxNOT_FOUND ) FontEnc=wxFONTENCODING_CP950;//BIG5
		else{
			for (unsigned i=0; i<=0xFF ; i++)
				bf[i] = (i< 0x20 || i==0x7F || i==0xAD) ? '.' : i;

			//Detecting Encoding
			char q=codepage[codepage.Find(wxT("CP125"))+5];

			//Filtering gaps
			if		 (q=='0') bf[0x81]=bf[0x83]=bf[0x88]=bf[0x90]=bf[0x98]='.';
			else if(q=='1') bf[0x98]='.';
			else if(q=='2') bf[0x81]=bf[0x8D]=bf[0x8F]=bf[0x90]=bf[0x9D]='.';
			else if(q=='3') bf[0x81]=bf[0x88]=bf[0x8A]=bf[0x8C]=bf[0x8D]=
								 bf[0x8E]=bf[0x8F]=bf[0x90]=bf[0x98]=bf[0x9A]=
								 bf[0x9C]=bf[0x9D]=bf[0x9E]=bf[0x9F]=bf[0xAA]=bf[0xD2]=bf[0xFF]='.';
			else if(q=='4') bf[0x81]=bf[0x8D]=bf[0x8E]=bf[0x8F]=bf[0x90]=bf[0x9D]=bf[0x9E]='.';
			else if(q=='5') bf[0x81]=bf[0x88]=bf[0x8A]=bf[0x8C]=bf[0x8D]=
								 bf[0x8E]=bf[0x8F]=bf[0x90]=bf[0x9A]=
								 bf[0x9C]=bf[0x9D]=bf[0x9E]=bf[0x9F]=bf[0xCA]=
								 bf[0xD9]=bf[0xDA]=bf[0xDB]=bf[0xDC]=bf[0xDD]=
								 bf[0xDE]=bf[0xDF]=bf[0xFB]=bf[0xFC]=bf[0xFF]='.';
			else if(q=='7') bf[0x81]=bf[0x83]=bf[0x88]=bf[0x8A]=bf[0x8C]=
								 bf[0x90]=bf[0x98]=bf[0x9A]=bf[0x9C]=bf[0x9F]=bf[0xA1]=bf[0xA5]='.' ;

			//Encoding
			if		(q=='0')		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_CP1250), 256);
			else if(q=='1')	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_CP1251), 256);
			else if(q=='2')	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_CP1252), 256);
			else if(q=='3')	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_CP1253), 256);
			else if(q=='4')	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_CP1254), 256);
			// Hebrew Output not looks good.
			else if(q=='5')	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_CP1255), 256);
			// Arabic Output from right issue!
			else if(q=='6')	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_CP1256), 256);
			else if(q=='7')	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_CP1257), 256);
			else if(q=='8'){ //Windows Vietnamese
				newCP=PrepareCodepageTable(wxT("CP1252")); //ANSI
				newCP[0x8A]=newCP[0x8E]=newCP[0x9A]=newCP[0x9E]='.';
				newCP[0xC3]=wxChar(0x0102);
				newCP[0xCC]='.';//wxChar(0x0300);
				newCP[0xD0]=wxChar(0x0110);
				newCP[0xD2]='.';//wxChar(0x0309);
				newCP[0xD5]=wxChar(0x01A0);
				newCP[0xDD]=wxChar(0x01AF);
				newCP[0xDE]='.';//wxChar(0x0303);
				newCP[0xE3]=wxChar(0x0103);
				newCP[0xEC]='.';//wxChar(0x0301);
				newCP[0xF0]=wxChar(0x0111);
				newCP[0xF2]='.';//wxChar(0x0323);
				newCP[0xF5]=wxChar(0x01A1);
				newCP[0xFD]=wxChar(0x01B0);
				newCP[0xFE]=wxChar(0x20AB);
				}
			}
		}

	else if(codepage.Find(wxT("AtariST")) != wxNOT_FOUND ){
		newCP+=PrepareCodepageTable(wxT("ASCII")).Mid(0x0,0x80);
		newCP+=PrepareCodepageTable(wxT("DOS CP437")).Mid(0x80,0x30);
		newCP += wxString::FromUTF8("\xC3\xA3\xC3\xB5\xC3\x98\xC3\xB8\xC5\x93\xC5\x92\xC3\x80\xC3\x83\xC3\x95\xC2\xA8\xC2\xB4\xE2\x80\xA0\xC2\xB6\xC2\xA9\xC2\xAE\xE2\x84\xA2"
			"\xC4\xB3\xC4\xB2\xD7\x90\xD7\x91\xD7\x92\xD7\x93\xD7\x94\xD7\x95\xD7\x96\xD7\x97\xD7\x98\xD7\x99\xD7\x9B\xD7\x9C\xD7\x9E\xD7\xA0"
			"\xD7\xA1\xD7\xA2\xD7\xA4\xD7\xA6\xD7\xA7\xD7\xA8\xD7\xA9\xD7\xAA\xD7\x9F\xD7\x9A\xD7\x9D\xD7\xA3\xD7\xA5\xC2\xA7\xE2\x88\xA7\xE2\x88\x9E");
		newCP += PrepareCodepageTable(wxT("DOS CP437")).Mid(0xE0, 0x20);
		newCP[0x9E]=wxChar(0x00DF);
		newCP[0xE1]=wxChar(0x03B2);
		newCP[0xEC]=wxChar(0x222E);
		newCP[0xEE]=wxChar(0x2208);
		newCP[0xFE]=wxChar(0x00B3);
		newCP[0xFF]=wxChar(0x00AF);
		}

	else if(codepage.Find(wxT("KOI7")) != wxNOT_FOUND ){
		newCP=PrepareCodepageTable(wxT("ASCII")).Mid(0,0x60);
		newCP += wxString::FromUTF8("\xD0\xAE\xD0\x90\xD0\x91\xD0\xA6\xD0\x94\xD0\x95\xD0\xA4\xD0\x93\xD0\xA5\xD0\x98\xD0\x99\xD0\x9A\xD0\x9B\xD0\x9C\xD0\x9D\xD0\x9E"
			"\xD0\x9F\xD0\xAF\xD0\xA0\xD0\xA1\xD0\xA2\xD0\xA3\xD0\x96\xD0\x92\xD0\xAC\xD0\xAB\xD0\x97\xD0\xA8\xD0\xAD\xD0\xA9\xD0\xA7.");
		for (unsigned i = 0x80; i <= 0xFF; i++)
			newCP += '.';
		}

	else if(codepage.Find(wxT("KOI8")) != wxNOT_FOUND ){
		for (unsigned i=0; i<=0xFF ; i++)
			bf[i] = (i<0x20 || i==0x7F)	? '.' : i;
		if(codepage.StartsWith(wxT("KOI8-R"))) newCP+=wxString( bf, wxCSConv(wxFONTENCODING_KOI8), 256);
		if(codepage.StartsWith(wxT("KOI8-U"))) newCP+=wxString( bf, wxCSConv(wxFONTENCODING_KOI8_U), 256);
		}

	else if(codepage.Find(wxT("JIS X 0201")) != wxNOT_FOUND ){
		for (unsigned i=0; i<0xFF ; i++)
			if(i==0x5C)
				newCP += wxChar(0xA5); //JPY
			else if(i==0x7E)
				newCP += wxChar(0x203E);//Overline
			else if(i<0x80)
				newCP += ((i<0x20 || i==0x7F)	? '.' : wxChar(i));
			else if( i>=0xA1 && i<0xE0)
				newCP += wxChar(i-0xA0+0xFF60);
			else
				newCP +='.';
		}

	else if(codepage.Find(wxT("TIS-620")) != wxNOT_FOUND ){
		newCP=PrepareCodepageTable(wxT("ISO/IEC 8859-11")); //Identical
		}

	else if(codepage.Find(wxT("EBCDIC")) != wxNOT_FOUND ){
		//Control chars replaced with dot
		for (unsigned i=0; i<0x40 ; i++)
			newCP+=wxChar('.');

		/// \x00AD (Soft Hypen) replaced with dot .
		/// \x009F (End Of File ) replaced with dot .
		//EBCDIC Table
		newCP += wxString::FromUTF8("\x20\xC2\xA0\xC3\xA2\xC3\xA4\xC3\xA0\xC3\xA1\xC3\xA3\xC3\xA5\xC3\xA7\xC3\xB1\xC2\xA2\x2E\x3C\x28\x2B\x7C"
			"\x26\xC3\xA9\xC3\xAA\xC3\xAB\xC3\xA8\xC3\xAD\xC3\xAE\xC3\xAF\xC3\xAC\xC3\x9F\x21\x24\x2A\x29\x3B\xC2\xAC"
			"\x2D\x2F\xC3\x82\xC3\x84\xC3\x80\xC3\x81\xC3\x83\xC3\x85\xC3\x87\xC3\x91\xC2\xA6\x2C\x25\x5F\x3E\x3F"
			"\xC3\xB8\xC3\x89\xC3\x8A\xC3\x8B\xC3\x88\xC3\x8D\xC3\x8E\xC3\x8F\xC3\x8C\x60\x3A\x23\x40\x27\x3D\x22"
			"\xC3\x98\x61\x62\x63\x64\x65\x66\x67\x68\x69\xC2\xAB\xC2\xBB\xC3\xB0\xC3\xBD\xC3\xBE\xC2\xB1"
			"\xC2\xB0\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\xC2\xAA\xC2\xBA\xC3\xA6\xC2\xB8\xC3\x86\xC2\xA4"
			"\xC2\xB5\x7E\x73\x74\x75\x76\x77\x78\x79\x7A\xC2\xA1\xC2\xBF\xC3\x90\xC3\x9D\xC3\x9E\xC2\xAE"
			"\x5E\xC2\xA3\xC2\xA5\xC2\xB7\xC2\xA9\xC2\xA7\xC2\xB6\xC2\xBC\xC2\xBD\xC2\xBE\x5B\x5D\xC2\xAF\xC2\xA8\xC2\xB4\xC3\x97"
			"\x7B\x41\x42\x43\x44\x45\x46\x47\x48\x49.\xC3\xB4\xC3\xB6\xC3\xB2\xC3\xB3\xC3\xB5"
			"\x7D\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\xC2\xB9\xC3\xBB\xC3\xBC\xC3\xB9\xC3\xBA\xC3\xBF"
			"\x5C\xC3\xB7\x53\x54\x55\x56\x57\x58\x59\x5A\xC2\xB2\xC3\x94\xC3\x96\xC3\x92\xC3\x93\xC3\x95"
			"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\xC2\xB3\xC3\x9B\xC3\x9C\xC3\x99\xC3\x9A.");

		if(codepage.Find(wxT("037")) != wxNOT_FOUND ){
			//This is EBCDIC 037	Already
			}

		else if(codepage.Find(wxT("285")) != wxNOT_FOUND ){
			unsigned char a[]="\x4A\x5B\xA1\xB0\xB1\xBA\xBC";  //Patch Index
			unsigned char b[]="\x24\xA3\xAF\xA2\x5B\x5E\x7E";	//Patch Value
			for(int i=0;i<7;i++)
				newCP[a[i]]=wxChar(b[i]);
			}

		else if(codepage.Find(wxT("424")) != wxNOT_FOUND ){
			newCP.Clear();
			for (unsigned i=0; i<0x40 ; i++)
				newCP+=wxChar('.');
			// At 0xFF, (0x9F) replaced with .
			newCP += wxString::FromUTF8("\x20\xD7\x90\xD7\x91\xD7\x92\xD7\x93\xD7\x94\xD7\x95\xD7\x96\xD7\x97\xD7\x98\xC2\xA2\x2E\x3C\x28\x2B\x7C"
				"\x26\xD7\x99\xD7\x9A\xD7\x9B\xD7\x9C\xD7\x9D\xD7\x9E\xD7\x9F\xD7\xA0\xD7\xA1\x21\x24\x2A\x29\x3B\xC2\xAC"
				"\x2D\x2F\xD7\xA2\xD7\xA3\xD7\xA4\xD7\xA5\xD7\xA6\xD7\xA7\xD7\xA8\xD7\xA9\xC2\xA6\x2C\x25\x5F\x3E\x3F"
				".\xD7\xAA..\xC2\xA0...\xE2\x80\x97\x60\x3A\x23\x40\x27\x3D\x22"
				".\x61\x62\x63\x64\x65\x66\x67\x68\x69\xC2\xAB\xC2\xBB...\xC2\xB1"
				"\xC2\xB0\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72...\xC2\xB8.\xC2\xA4"
				"\xC2\xB5\x7E\x73\x74\x75\x76\x77\x78\x79\x7A.....\xC2\xAE"
				"\x5E\xC2\xA3\xC2\xA5\xC2\xB7\xC2\xA9\xC2\xA7\xC2\xB6\xC2\xBC\xC2\xBD\xC2\xBE\x5B\x5D\xC2\xAF\xC2\xA8\xC2\xB4\xC3\x97"
				"\x7B\x41\x42\x43\x44\x45\x46\x47\x48\x49......"
				"\x7D\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\xC2\xB9....."
				"\x5C\xC3\xB7\x53\x54\x55\x56\x57\x58\x59\x5A\xC2\xB2....."
				"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\xC2\xB3.....");
		}


		else if(codepage.Find(wxT("500")) != wxNOT_FOUND ){
			unsigned char a[]="\x4A\x4F\x5A\x5F\xB0\xBA\xBB\xCA"; //Patch Index
			unsigned char b[]="\x5B\x21\x5D\x5E\xA2\xAC\x7C\x2E";	//Patch Value
			for(int i=0;i<8;i++)
				newCP[a[i]]=wxChar(b[i]);
			}

		else if(codepage.Find(wxT("875")) != wxNOT_FOUND ){
			newCP=wxEmptyString;
			for (unsigned i=0; i<0x40 ; i++)
				newCP+=wxChar('.');
			// At 0xCA, (0xAD) replaced with .
			// At 0xFF, (0x9F) replaced with .
			// Others are 0x1A originaly, replaced with .
			newCP += wxString::FromUTF8("\x20\xCE\x91\xCE\x92\xCE\x93\xCE\x94\xCE\x95\xCE\x96\xCE\x97\xCE\x98\xCE\x99\x5B\x2E\x3C\x28\x2B\x21"
				"\x26\xCE\x9A\xCE\x9B\xCE\x9C\xCE\x9D\xCE\x9E\xCE\x9F\xCE\xA0\xCE\xA1\xCE\xA3\x5D\x24\x2A\x29\x3B\x5E"
				"\x2D\x2F\xCE\xA4\xCE\xA5\xCE\xA6\xCE\xA7\xCE\xA8\xCE\xA9\xCE\xAA\xCE\xAB\x7C\x2C\x25\x5F\x3E\x3F"
				"\xC2\xA8\xCE\x86\xCE\x88\xCE\x89\xC2\xA0\xCE\x8A\xCE\x8C\xCE\x8E\xCE\x8F\x60\x3A\x23\x40\x27\x3D\x22"
				"\xCE\x85\x61\x62\x63\x64\x65\x66\x67\x68\x69\xCE\xB1\xCE\xB2\xCE\xB3\xCE\xB4\xCE\xB5\xCE\xB6"
				"\xC2\xB0\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\xCE\xB7\xCE\xB8\xCE\xB9\xCE\xBA\xCE\xBB\xCE\xBC"
				"\xC2\xB4\x7E\x73\x74\x75\x76\x77\x78\x79\x7A\xCE\xBD\xCE\xBE\xCE\xBF\xCF\x80\xCF\x81\xCF\x83"
				"\xC2\xA3\xCE\xAC\xCE\xAD\xCE\xAE\xCF\x8A\xCE\xAF\xCF\x8C\xCF\x8D\xCF\x8B\xCF\x8E\xCF\x82\xCF\x84\xCF\x85\xCF\x86\xCF\x87\xCF\x88"
				"\x7B\x41\x42\x43\x44\x45\x46\x47\x48\x49.\xCF\x89\xCE\x90\xCE\xB0\xE2\x80\x98\xE2\x80\x95"
				"\x7D\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\xC2\xB1\xC2\xBD.\xCE\x87\xE2\x80\x99\xC2\xA6"
				"\x5C.\x53\x54\x55\x56\x57\x58\x59\x5A\xC2\xB2\xC2\xA7..\xC2\xAB\xC2\xAC"
				"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\xC2\xB3\xC2\xA9..\xC2\xBB.");
		}

		else if(codepage.Find(wxT("1026")) != wxNOT_FOUND ){
			newCP=wxEmptyString;
			for (unsigned i=0; i<0x40 ; i++)
				newCP+=wxChar('.');
			// At 0xCA, (0xAD) replaced with .
			// At 0xFF, (0x9F) replaced with .
			newCP += wxString::FromUTF8("\x20\xC2\xA0\xC3\xA2\xC3\xA4\xC3\xA0\xC3\xA1\xC3\xA3\xC3\xA5\x7B\xC3\xB1\xC3\x87\x2E\x3C\x28\x2B\x21"
				"\x26\xC3\xA9\xC3\xAA\xC3\xAB\xC3\xA8\xC3\xAD\xC3\xAE\xC3\xAF\xC3\xAC\xC3\x9F\xC4\x9E\xC4\xB0\x2A\x29\x3B\x5E"
				"\x2D\x2F\xC3\x82\xC3\x84\xC3\x80\xC3\x81\xC3\x83\xC3\x85\x5B\xC3\x91\xC5\x9F\x2C\x25\x5F\x3E\x3F"
				"\xC3\xB8\xC3\x89\xC3\x8A\xC3\x8B\xC3\x88\xC3\x8D\xC3\x8E\xC3\x8F\xC3\x8C\xC4\xB1\x3A\xC3\x96\xC5\x9E\x27\x3D\xC3\x9C"
				"\xC3\x98\x61\x62\x63\x64\x65\x66\x67\x68\x69\xC2\xAB\xC2\xBB\x7D\x60\xC2\xA6\xC2\xB1"
				"\xC2\xB0\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\xC2\xAA\xC2\xBA\xC3\xA6\xC2\xB8\xC3\x86\xC2\xA4"
				"\xC2\xB5\xC3\xB6\x73\x74\x75\x76\x77\x78\x79\x7A\xC2\xA1\xC2\xBF\x5D\x24\x40\xC2\xAE"
				"\xC2\xA2\xC2\xA3\xC2\xA5\xC2\xB7\xC2\xA9\xC2\xA7\xC2\xB6\xC2\xBC\xC2\xBD\xC2\xBE\xC2\xAC\x7C\xC2\xAF\xC2\xA8\xC2\xB4\xC3\x97"
				"\xC3\xA7\x41\x42\x43\x44\x45\x46\x47\x48\x49.\xC3\xB4\x7E\xC3\xB2\xC3\xB3\xC3\xB5"
				"\xC4\x9F\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\xC2\xB9\xC3\xBB\x5C\xC3\xB9\xC3\xBA\xC3\xBF"
				"\xC3\xBC\xC3\xB7\x53\x54\x55\x56\x57\x58\x59\x5A\xC2\xB2\xC3\x94\x23\xC3\x92\xC3\x93\xC3\x95"
				"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\xC2\xB3\xC3\x9B\x22\xC3\x99\xC3\x9A.");

			}

		else if(codepage.Find(wxT("1047")) != wxNOT_FOUND ){
			unsigned char a[]="\x8F\xAD\xB0\xBA\xBB\xBD";  //Patch Index
			unsigned char b[]="\x5E\x5B\xAC\xDD\xA8\x5D";  //Patch Value
			for(int i=0;i<6;i++)
				newCP[a[i]]=wxChar(b[i]);
			}
		else if(codepage.Find(wxT("1040")) != wxNOT_FOUND ){
			newCP[0x9F]=wxChar(0x20AC);
			}

		else if(codepage.Find(wxT("1146")) != wxNOT_FOUND ){
			newCP=PrepareCodepageTable( wxT("EBCDIC 285"));
			newCP[0x9F]=wxChar(0x20AC);
			}

		else if(codepage.Find(wxT("1148")) != wxNOT_FOUND ){
			newCP=PrepareCodepageTable( wxT("EBCDIC 500"));
			newCP[0x9F]=wxChar(0x20AC);
			}
		}

	else if(codepage.Find(wxT("Macintosh")) != wxNOT_FOUND ){
		//Control chars replaced with dot
		for (unsigned i=0; i<0x20 ; i++)
			newCP+=wxChar('.');
		//ASCII compatible part
		for( unsigned i=0x20 ; i < 0x7F ; i++ )
			newCP += wxChar(i);
		newCP+='.';//0xFF delete char

		if( codepage.Find(wxT("CP10000")) != wxNOT_FOUND )		//Macintosh Roman extension table
			newCP += wxString::FromUTF8("\xC3\x84\xC3\x85\xC3\x87\xC3\x89\xC3\x91\xC3\x96\xC3\x9C\xC3\xA1\xC3\xA0\xC3\xA2\xC3\xA4\xC3\xA3\xC3\xA5\xC3\xA7\xC3\xA9\xC3\xA8"
			"\xC3\xAA\xC3\xAB\xC3\xAD\xC3\xAC\xC3\xAE\xC3\xAF\xC3\xB1\xC3\xB3\xC3\xB2\xC3\xB4\xC3\xB6\xC3\xB5\xC3\xBA\xC3\xB9\xC3\xBB\xC3\xBC"
			"\xE2\x80\xA0\xC2\xB0\xC2\xA2\xC2\xA3\xC2\xA7\xE2\x80\xA2\xC2\xB6\xC3\x9F\xC2\xAE\xC2\xA9\xE2\x84\xA2\xC2\xB4\xC2\xA8\xE2\x89\xA0\xC3\x86\xC3\x98"
			"\xE2\x88\x9E\xC2\xB1\xE2\x89\xA4\xE2\x89\xA5\xC2\xA5\xC2\xB5\xE2\x88\x82\xE2\x88\x91\xE2\x88\x8F\xCF\x80\xE2\x88\xAB\xC2\xAA\xC2\xBA\xE2\x84\xA6\xC3\xA6\xC3\xB8"
			"\xC2\xBF\xC2\xA1\xC2\xAC\xE2\x88\x9A\xC6\x92\xE2\x89\x88\xE2\x88\x86\xC2\xAB\xC2\xBB\xE2\x80\xA6\xC2\xA0\xC3\x80\xC3\x83\xC3\x95\xC5\x92\xC5\x93"
			"\xE2\x80\x93\xE2\x80\x94\xE2\x80\x9C\xE2\x80\x9D\xE2\x80\x98\xE2\x80\x99\xC3\xB7\xE2\x97\x8A\xC3\xBF\xC5\xB8\xE2\x81\x84\xC2\xA4\xE2\x80\xB9\xE2\x80\xBA\xEF\xAC\x81\xEF\xAC\x82"
			"\xE2\x80\xA1\xC2\xB7\xE2\x80\x9A\xE2\x80\x9E\xE2\x80\xB0\xC3\x82\xC3\x8A\xC3\x81\xC3\x8B\xC3\x88\xC3\x8D\xC3\x8E\xC3\x8F\xC3\x8C\xC3\x93\xC3\x94"
			".\xC3\x92\xC3\x9A\xC3\x9B\xC3\x99\xC4\xB1\xCB\x86\xCB\x9C\xC2\xAF\xCB\x98\xCB\x99\xCB\x9A\xC2\xB8\xCB\x9D\xCB\x9B\xCB\x87");

		else if( codepage.Find(wxT("CP10029")) != wxNOT_FOUND )		//Macintosh Latin2 extension table
			newCP += wxString::FromUTF8("\xC3\x84\xC4\x80\xC4\x81\xC3\x89\xC4\x84\xC3\x96\xC3\x9C\xC3\xA1\xC4\x85\xC4\x8C\xC3\xA4\xC4\x8D\xC4\x86\xC4\x87\xC3\xA9\xC5\xB9"
			"\xC5\xBA\xC4\x8E\xC3\xAD\xC4\x8F\xC4\x92\xC4\x93\xC4\x96\xC3\xB3\xC4\x97\xC3\xB4\xC3\xB6\xC3\xB5\xC3\xBA\xC4\x9A\xC4\x9B\xC3\xBC"
			"\xE2\x80\xA0\xC2\xB0\xC4\x98\xC2\xA3\xC2\xA7\xE2\x80\xA2\xC2\xB6\xC3\x9F\xC2\xAE\xC2\xA9\xE2\x84\xA2\xC4\x99\xC2\xA8\xE2\x89\xA0\xC4\xA3\xC4\xAE"
			"\xC4\xAF\xC4\xAA\xE2\x89\xA4\xE2\x89\xA5\xC4\xAB\xC4\xB6\xE2\x88\x82\xE2\x88\x91\xC5\x82\xC4\xBB\xC4\xBC\xC4\xBD\xC4\xBE\xC4\xB9\xC4\xBA\xC5\x85"
			"\xC5\x86\xC5\x83\xC2\xAC\xE2\x88\x9A\xC5\x84\xC5\x87\xE2\x88\x86\xC2\xAB\xC2\xBB\xE2\x80\xA6\xC2\xA0\xC5\x88\xC5\x90\xC3\x95\xC5\x91\xC5\x8C"
			"\xE2\x80\x93\xE2\x80\x94\xE2\x80\x9C\xE2\x80\x9D\xE2\x80\x98\xE2\x80\x99\xC3\xB7\xE2\x97\x8A\xC5\x8D\xC5\x94\xC5\x95\xC5\x98\xE2\x80\xB9\xE2\x80\xBA\xC5\x99\xC5\x96"
			"\xC5\x97\xC5\xA0\xE2\x80\x9A\xE2\x80\x9E\xC5\xA1\xC5\x9A\xC5\x9B\xC3\x81\xC5\xA4\xC5\xA5\xC3\x8D\xC5\xBD\xC5\xBE\xC5\xAA\xC3\x93\xC3\x94"
			"\xC5\xAB\xC5\xAE\xC3\x9A\xC5\xAF\xC5\xB0\xC5\xB1\xC5\xB2\xC5\xB3\xC3\x9D\xC3\xBD\xC4\xB7\xC5\xBB\xC5\x81\xC5\xBC\xC4\xA2\xCB\x87");


		else if( codepage.Find(wxT("CP10079")) != wxNOT_FOUND )		//Macintosh Icelandic extension table
			newCP += wxString::FromUTF8("\xC3\x84\xC3\x85\xC3\x87\xC3\x89\xC3\x91\xC3\x96\xC3\x9C\xC3\xA1\xC3\xA0\xC3\xA2\xC3\xA4\xC3\xA3\xC3\xA5\xC3\xA7\xC3\xA9\xC3\xA8"
			"\xC3\xAA\xC3\xAB\xC3\xAD\xC3\xAC\xC3\xAE\xC3\xAF\xC3\xB1\xC3\xB3\xC3\xB2\xC3\xB4\xC3\xB6\xC3\xB5\xC3\xBA\xC3\xB9\xC3\xBB\xC3\xBC"
			"\xC3\x9D\xC2\xB0\xC2\xA2\xC2\xA3\xC2\xA7\xE2\x80\xA2\xC2\xB6\xC3\x9F\xC2\xAE\xC2\xA9\xE2\x84\xA2\xC2\xB4\xC2\xA8\xE2\x89\xA0\xC3\x86\xC3\x98"
			"\xE2\x88\x9E\xC2\xB1\xE2\x89\xA4\xE2\x89\xA5\xC2\xA5\xC2\xB5\xE2\x88\x82\xE2\x88\x91\xE2\x88\x8F\xCF\x80\xE2\x88\xAB\xC2\xAA\xC2\xBA\xE2\x84\xA6\xC3\xA6\xC3\xB8"
			"\xC2\xBF\xC2\xA1\xC2\xAC\xE2\x88\x9A\xC6\x92\xE2\x89\x88\xE2\x88\x86\xC2\xAB\xC2\xBB\xE2\x80\xA6\xC2\xA0\xC3\x80\xC3\x83\xC3\x95\xC5\x92\xC5\x93"
			"\xE2\x80\x93\xE2\x80\x94\xE2\x80\x9C\xE2\x80\x9D\xE2\x80\x98\xE2\x80\x99\xC3\xB7\xE2\x97\x8A\xC3\xBF\xC5\xB8\xE2\x81\x84\xC2\xA4\xC3\x90\xC3\xB0\xC3\x9E\xC3\xBE"
			"\xC3\xBD\xC2\xB7\xE2\x80\x9A\xE2\x80\x9E\xE2\x80\xB0\xC3\x82\xC3\x8A\xC3\x81\xC3\x8B\xC3\x88\xC3\x8D\xC3\x8E\xC3\x8F\xC3\x8C\xC3\x93\xC3\x94"
			".\xC3\x92\xC3\x9A\xC3\x9B\xC3\x99\xC4\xB1\xCB\x86\xCB\x9C\xC2\xAF\xCB\x98\xCB\x99\xCB\x9A\xC2\xB8\xCB\x9D\xCB\x9B\xCB\x87");


		else if( codepage.Find(wxT("CP10006")) != wxNOT_FOUND )		//Macintosh Greek CP10006 extension table
			newCP += wxString::FromUTF8("\xC3\x84\xC2\xB9\xC2\xB2\xC3\x89\xC2\xB3\xC3\x96\xC3\x9C\xCE\x85\xC3\xA0\xC3\xA2\xC3\xA4\xCE\x84\xC2\xA8\xC3\xA7\xC3\xA9\xC3\xA8"
			"\xC3\xAA\xC3\xAB\xC2\xA3\xE2\x84\xA2\xC3\xAE\xC3\xAF\xE2\x80\xA2\xC2\xBD\xE2\x80\xB0\xC3\xB4\xC3\xB6\xC2\xA6\xC2\xAD\xC3\xB9\xC3\xBB\xC3\xBC"
			"\xE2\x80\xA0\xCE\x93\xCE\x94\xCE\x98\xCE\x9B\xCE\x9E\xCE\xA0\xC3\x9F\xC2\xAE\xC2\xA9\xCE\xA3\xCE\xAA\xC2\xA7\xE2\x89\xA0\xC2\xB0\xCE\x87"
			"\xCE\x91\xC2\xB1\xE2\x89\xA4\xE2\x89\xA5\xC2\xA5\xCE\x92\xCE\x95\xCE\x96\xCE\x97\xCE\x99\xCE\x9A\xCE\x9C\xCE\xA6\xCE\xAB\xCE\xA8\xCE\xA9"
			"\xCE\xAC\xCE\x9D\xC2\xAC\xCE\x9F\xCE\xA1\xE2\x89\x88\xCE\xA4\xC2\xAB\xC2\xBB\xE2\x80\xA6\xC2\xA0\xCE\xA5\xCE\xA7\xCE\x86\xCE\x88\xC5\x93"
			"\xE2\x80\x93\xE2\x80\x95\xE2\x80\x9C\xE2\x80\x9D\xE2\x80\x98\xE2\x80\x99\xC3\xB7\xCE\x89\xCE\x8A\xCE\x8C\xCE\x8E\xCE\xAD\xCE\xAE\xCE\xAF\xCF\x8C\xCE\x8F"
			"\xCF\x8D\xCE\xB1\xCE\xB2\xCF\x88\xCE\xB4\xCE\xB5\xCF\x86\xCE\xB3\xCE\xB7\xCE\xB9\xCE\xBE\xCE\xBA\xCE\xBB\xCE\xBC\xCE\xBD\xCE\xBF"
			"\xCF\x80\xCF\x8E\xCF\x81\xCF\x83\xCF\x84\xCE\xB8\xCF\x89\xCF\x82\xCF\x87\xCF\x85\xCE\xB6\xCF\x8A\xCF\x8B\xCE\x90\xCE\xB0.");

		else if( codepage.Find(wxT("CP10007")) != wxNOT_FOUND )//Macintosh Cyrillic extension table
			newCP += wxString::FromUTF8("\xD0\x90\xD0\x91\xD0\x92\xD0\x93\xD0\x94\xD0\x95\xD0\x96\xD0\x97\xD0\x98\xD0\x99\xD0\x9A\xD0\x9B\xD0\x9C\xD0\x9D\xD0\x9E\xD0\x9F"
			"\xD0\xA0\xD0\xA1\xD0\xA2\xD0\xA3\xD0\xA4\xD0\xA5\xD0\xA6\xD0\xA7\xD0\xA8\xD0\xA9\xD0\xAA\xD0\xAB\xD0\xAC\xD0\xAD\xD0\xAE\xD0\xAF"
			"\xE2\x80\xA0\xC2\xB0\xC2\xA2\xC2\xA3\xC2\xA7\xE2\x80\xA2\xC2\xB6\xD0\x86\xC2\xAE\xC2\xA9\xE2\x84\xA2\xD0\x82\xD1\x92\xE2\x89\xA0\xD0\x83\xD1\x93"
			"\xE2\x88\x9E\xC2\xB1\xE2\x89\xA4\xE2\x89\xA5\xD1\x96\xC2\xB5\xE2\x88\x82\xD0\x88\xD0\x84\xD1\x94\xD0\x87\xD1\x97\xD0\x89\xD1\x99\xD0\x8A\xD1\x9A"
			"\xD1\x98\xD0\x85\xC2\xAC\xE2\x88\x9A\xC6\x92\xE2\x89\x88\xE2\x88\x86\xC2\xAB\xC2\xBB\xE2\x80\xA6\xC2\xA0\xD0\x8B\xD1\x9B\xD0\x8C\xD1\x9C\xD1\x95"
			"\xE2\x80\x93\xE2\x80\x94\xE2\x80\x9C\xE2\x80\x9D\xE2\x80\x98\xE2\x80\x99\xC3\xB7\xE2\x80\x9E\xD0\x8E\xD1\x9E\xD0\x8F\xD1\x9F\xE2\x84\x96\xD0\x81\xD1\x91\xD1\x8F"
			"\xD0\xB0\xD0\xB1\xD0\xB2\xD0\xB3\xD0\xB4\xD0\xB5\xD0\xB6\xD0\xB7\xD0\xB8\xD0\xB9\xD0\xBA\xD0\xBB\xD0\xBC\xD0\xBD\xD0\xBE\xD0\xBF"
			"\xD1\x80\xD1\x81\xD1\x82\xD1\x83\xD1\x84\xD1\x85\xD1\x86\xD1\x87\xD1\x88\xD1\x89\xD1\x8A\xD1\x8B\xD1\x8C\xD1\x8D\xD1\x8E\xC2\xA4");

		else if( codepage.Find(wxT("CP10081")) != wxNOT_FOUND )		//Macintosh Turkish extension table
			newCP += wxString::FromUTF8("\xC3\x84\xC3\x85\xC3\x87\xC3\x89\xC3\x91\xC3\x96\xC3\x9C\xC3\xA1\xC3\xA0\xC3\xA2\xC3\xA4\xC3\xA3\xC3\xA5\xC3\xA7\xC3\xA9\xC3\xA8"
			"\xC3\xAA\xC3\xAB\xC3\xAD\xC3\xAC\xC3\xAE\xC3\xAF\xC3\xB1\xC3\xB3\xC3\xB2\xC3\xB4\xC3\xB6\xC3\xB5\xC3\xBA\xC3\xB9\xC3\xBB\xC3\xBC"
			"\xE2\x80\xA0\xC2\xB0\xC2\xA2\xC2\xA3\xC2\xA7\xE2\x80\xA2\xC2\xB6\xC3\x9F\xC2\xAE\xC2\xA9\xE2\x84\xA2\xC2\xB4\xC2\xA8\xE2\x89\xA0\xC3\x86\xC3\x98"
			"\xE2\x88\x9E\xC2\xB1\xE2\x89\xA4\xE2\x89\xA5\xC2\xA5\xC2\xB5\xE2\x88\x82\xE2\x88\x91\xE2\x88\x8F\xCF\x80\xE2\x88\xAB\xC2\xAA\xC2\xBA\xE2\x84\xA6\xC3\xA6\xC3\xB8"
			"\xC2\xBF\xC2\xA1\xC2\xAC\xE2\x88\x9A\xC6\x92\xE2\x89\x88\xE2\x88\x86\xC2\xAB\xC2\xBB\xE2\x80\xA6\xC2\xA0\xC3\x80\xC3\x83\xC3\x95\xC5\x92\xC5\x93"
			"\xE2\x80\x93\xE2\x80\x94\xE2\x80\x9C\xE2\x80\x9D\xE2\x80\x98\xE2\x80\x99\xC3\xB7\xE2\x97\x8A\xC3\xBF\xC5\xB8\xC4\x9E\xC4\x9F\xC4\xB0\xC4\xB1\xC5\x9E\xC5\x9F"
			"\xE2\x80\xA1\xC2\xB7\xE2\x80\x9A\xE2\x80\x9E\xE2\x80\xB0\xC3\x82\xC3\x8A\xC3\x81\xC3\x8B\xC3\x88\xC3\x8D\xC3\x8E\xC3\x8F\xC3\x8C\xC3\x93\xC3\x94"
			".\xC3\x92\xC3\x9A\xC3\x9B\xC3\x99.\xCB\x86\xCB\x9C\xC2\xAF\xCB\x98\xCB\x99\xCB\x9A\xC2\xB8\xCB\x9D\xCB\x9B\xCB\x87");

		#ifdef __WXMAC__
		else{
			for (unsigned i=0; i<=0xFF ; i++)
				bf[i] = (i<0x20 || i==0x7F)	? '.' : i;
			if( codepage.Find(wxT("Arabic")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACARABIC), 256);
			if( codepage.Find(wxT("Arabic Ext")) != wxNOT_FOUND )	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACARABICEXT), 256);
			if( codepage.Find(wxT("Armanian")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACARMENIAN), 256);
			if( codepage.Find(wxT("Bengali")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACBENGALI), 256);
			if( codepage.Find(wxT("Burmese")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACBURMESE), 256);
			if( codepage.Find(wxT("Celtic")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACCELTIC), 256);
			if( codepage.Find(wxT("Central European")) != wxNOT_FOUND )	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACCENTRALEUR), 256);
			if( codepage.Find(wxT("Chinese Imperial")) != wxNOT_FOUND )	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACCHINESESIMP), 256);
			if( codepage.Find(wxT("Chinese Traditional")) != wxNOT_FOUND )	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACCHINESETRAD), 256);
			if( codepage.Find(wxT("Croatian")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACCROATIAN), 256);
			if( codepage.Find(wxT("Cyrillic")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACCYRILLIC), 256);
			if( codepage.Find(wxT("Devanagari")) != wxNOT_FOUND )	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACDEVANAGARI), 256);
			if( codepage.Find(wxT("Dingbats")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACDINGBATS), 256);
			if( codepage.Find(wxT("Ethiopic")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACETHIOPIC), 256);
			if( codepage.Find(wxT("Gaelic")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACGAELIC), 256);
			if( codepage.Find(wxT("Georgian")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACGEORGIAN), 256);
			if( codepage.Find(wxT("Greek")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACGREEK), 256);
			if( codepage.Find(wxT("Gujarati")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACGUJARATI), 256);
			if( codepage.Find(wxT("Gurmukhi")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACGURMUKHI), 256);
			if( codepage.Find(wxT("Hebrew")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACHEBREW), 256);
			if( codepage.Find(wxT("Icelandic")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACICELANDIC), 256);
			if( codepage.Find(wxT("Japanese")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACJAPANESE), 256);
			if( codepage.Find(wxT("Kannada")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACKANNADA), 256);
			if( codepage.Find(wxT("Keyboard")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACKEYBOARD), 256);
			if( codepage.Find(wxT("Khmer")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACKHMER), 256);
			if( codepage.Find(wxT("Korean")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACKOREAN), 256);
			if( codepage.Find(wxT("Laotian")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACLAOTIAN), 256);
			if( codepage.Find(wxT("Malajalam")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACMALAJALAM), 256);
//			if( codepage.Find(wxT("Min")) != wxNOT_FOUND )	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACMAX), 256);
//			if( codepage.Find(wxT("Max")) != wxNOT_FOUND )	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACMIN), 256);
			if( codepage.Find(wxT("Mongolian")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACMONGOLIAN), 256);
			if( codepage.Find(wxT("Oriya")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACORIYA), 256);
			if( codepage.Find(wxT("Roman ")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACROMAN), 256);
			if( codepage.Find(wxT("Romanian")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACROMANIAN), 256);
			if( codepage.Find(wxT("Sinhalese")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACSINHALESE), 256);
			if( codepage.Find(wxT("Symbol")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACSYMBOL), 256);
			if( codepage.Find(wxT("Tamil")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACTAMIL), 256);
			if( codepage.Find(wxT("Telugu")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACTELUGU), 256);
			if( codepage.Find(wxT("Thai")) != wxNOT_FOUND )			newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACTHAI), 256);
			if( codepage.Find(wxT("Tibetan")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACTIBETAN), 256);
			if( codepage.Find(wxT("Turkish")) != wxNOT_FOUND )		newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACTURKISH), 256);
			if( codepage.Find(wxT("Viatnamese")) != wxNOT_FOUND )	newCP+=wxString( bf, wxCSConv(wxFONTENCODING_MACVIATNAMESE), 256);
			}
		#endif// __WXMAC__
		}

	else if( codepage.Find(wxT("DEC Multinational")) != wxNOT_FOUND ){
		newCP=PrepareCodepageTable(wxT("ISO/IEC 8859-1 "));//Warning! Watch the space after last 1
		unsigned char a[]="\xA0\xA4\xA6\xA8\xAC\xAD\xAE\xAF\xB4\xB8\xBE\xD0\xDE\xF0\xFD\xFE\xFF";  //Patch Index
		unsigned char b[]="...\xA4..........\xFF..";	//Patch Value
		for(int i=0;i<17;i++)
			newCP[a[i]]=wxChar(b[i]);

		newCP[0xD7]=wxChar(0x0152);
		newCP[0xDD]=wxChar(0x0178);
		newCP[0xF7]=wxChar(0x0153);
		}

	else if(codepage.Find(wxT("UTF8 ")) != wxNOT_FOUND )		FontEnc=wxFONTENCODING_UTF8;
	else if(codepage.Find(wxT("UTF16 ")) != wxNOT_FOUND )		FontEnc=wxFONTENCODING_UTF16;
	else if(codepage.Find(wxT("UTF16LE")) != wxNOT_FOUND )	FontEnc=wxFONTENCODING_UTF16LE;
	else if(codepage.Find(wxT("UTF16BE")) != wxNOT_FOUND )	FontEnc=wxFONTENCODING_UTF16BE;
	else if(codepage.Find(wxT("UTF32 ")) != wxNOT_FOUND )		FontEnc=wxFONTENCODING_UTF32;
	else if(codepage.Find(wxT("UTF32LE")) != wxNOT_FOUND )	FontEnc=wxFONTENCODING_UTF32LE;
	else if(codepage.Find(wxT("UTF32BE")) != wxNOT_FOUND )	FontEnc=wxFONTENCODING_UTF32BE;
	else if(codepage.Find(wxT("GB2312")) != wxNOT_FOUND )	FontEnc=wxFONTENCODING_GB2312;
	else if(codepage.Find(wxT("GBK")) != wxNOT_FOUND )		FontEnc=wxFONTENCODING_CP936;
	else if(codepage.Find(wxT("Shift JIS")) != wxNOT_FOUND )FontEnc=wxFONTENCODING_SHIFT_JIS;//CP932
	else if(codepage.Find(wxT("Big5")) != wxNOT_FOUND )		FontEnc=wxFONTENCODING_BIG5;//CP950
	else if(codepage.Find(wxT("EUC-JP")) != wxNOT_FOUND )	FontEnc=wxFONTENCODING_EUC_JP;
	else if(codepage.Find(wxT("EUC-KR")) != wxNOT_FOUND )	FontEnc=wxFONTENCODING_CP949; //EUC-KR
//	else if(codepage.StartsWith(wxT("EUC-CN")))		FontEnc=wxFONTENCODING_GB2312;
//else if(codepage.Find(wxT("Linux Bulgarian")) != wxNOT_FOUND )		FontEnc=wxFONTENCODING_BULGARIAN;
	return CodepageTable=newCP;
	}

void wxHexTextCtrl::Replace(unsigned text_location, const wxChar& value, bool paint){
	if( text_location < m_text.Length() )
		m_text[text_location] = Filter(value);
//		m_text[text_location] = value;
	else{
		m_text << Filter(value);
//		m_text << value;
		//m_text << wxT("0");
		}
	RePaint();
	}

void wxHexTextCtrl::ChangeValue( const wxString& value, bool paint ){
	m_text = value;
	if( paint )
		RePaint();
	}

void wxHexTextCtrl::SetBinValue( char* buffer, int len, bool paint ){
	m_text.Clear();

	if(FontEnc!=wxFONTENCODING_ALTERNATIVE){
		///only shows the annoying pop-up and does not do anything else.
		//if(not wxFontMapper::Get()->IsEncodingAvailable( FontEnc ) )
		//	wxMessageBox(wxT("Encoding is not available!"), wxT("Error!"), wxOK|wxCENTRE );
		m_text << FilterMBBuffer(buffer,len,FontEnc);
		}
	else
		for( int i=0 ; i<len ; i++ )
			m_text << Filter(buffer[i]);

//	m_text << FilterUTF8(buffer,len);
//	m_text=wxString(buffer, wxCSConv(wxFONTENCODING_CP1252),  len);
//	m_text=wxString(buffer, wxCSConv(wxFONTENCODING_UTF8),  len);

	if( paint )
		RePaint();
	}

void wxHexTextCtrl::SetDefaultStyle( wxTextAttr& new_attr ){
	HexDefaultAttr = new_attr;

	wxClientDC dc(this);
	dc.SetFont( HexDefaultAttr.GetFont() );
	SetFont( HexDefaultAttr.GetFont() );
	m_CharSize.y = dc.GetCharHeight();
	m_CharSize.x = dc.GetCharWidth();
	if( FontEnc == wxFONTENCODING_UTF16LE ||
		 FontEnc == wxFONTENCODING_UTF16BE ||
		 FontEnc == wxFONTENCODING_UTF32LE ||
		 FontEnc == wxFONTENCODING_UTF32BE
		 ){
	   m_CharSize.x += 5;
		DrawCharByChar=true;
		}
	else
		DrawCharByChar=false;

	wxCaret *caret = GetCaret();
#ifdef _DEBUG_CARET_
	std::cout << "Caret = 0x"<< (intptr_t) caret <<  " - mycaret= 0x" << (intptr_t) mycaret << "m_charSize.x" << m_CharSize.x << std::endl;
#endif
	if ( caret )
		caret->SetSize(m_CharSize.x, m_CharSize.y);

	RePaint();
}

int wxHexTextCtrl::PixelCoordToInternalPosition( wxPoint mouse ){
	mouse.x = ( mouse.x < 0 ? 0 : mouse.x);
	mouse.x = ( mouse.x > m_CharSize.x*m_Window.x ? m_CharSize.x*m_Window.x-1 : mouse.x);
	mouse.y = ( mouse.y < 0 ? 0 : mouse.y);
	mouse.y = ( mouse.y > m_CharSize.y*m_Window.y ? m_CharSize.y*m_Window.y-1 : mouse.y);

	int x = (mouse.x - m_Margin.x) / m_CharSize.x;
	int y = (mouse.y - m_Margin.y) / m_CharSize.y;
	return ( x + y * CharacterPerLine() );
	}

int wxHexTextCtrl::GetInsertionPoint( void ){
	return m_Caret.x  + CharacterPerLine() * m_Caret.y;
	}

void wxHexTextCtrl::SetInsertionPoint( unsigned int pos ){
	if(pos > m_text.Length())
		pos = m_text.Length();
	MoveCaret( wxPoint(pos%m_Window.x , pos/m_Window.x) );
	}

void wxHexTextCtrl::ChangeSize(){
	unsigned gip = GetInsertionPoint();
	wxSize size = GetClientSize();

	m_Window.x = (size.x - 2*m_Margin.x) / m_CharSize.x;
	m_Window.y = (size.y - 2*m_Margin.x) / m_CharSize.y;
	if ( m_Window.x < 1 )
		m_Window.x = 1;
	if ( m_Window.y < 1 )
		m_Window.y = 1;

	//This Resizes internal buffer!
	CreateDC();

	RePaint();
	SetInsertionPoint( gip );
	}

void wxHexTextCtrl::DrawCursorShadow(wxDC* dcTemp){
	if( m_Window.x <= 0 ||
		FindFocus()==this )
		return;

	#ifdef _DEBUG_CARET_
	std::cout << "wxHexTextCtrl::DrawCursorShadow(x,y) - charsize(x,y) :" << m_Caret.x << ',' <<  m_Caret.y << " - " << m_CharSize.x << ',' << m_CharSize.y << std::endl;
	#endif // _DEBUG_CARET_

	int y=m_CharSize.y*( m_Caret.y ) + m_Margin.y;
	int x=m_CharSize.x*( m_Caret.x ) + m_Margin.x;

	dcTemp->SetPen( *wxBLACK_PEN );
	dcTemp->SetBrush( *wxTRANSPARENT_BRUSH );
	dcTemp->DrawRectangle(x,y,m_CharSize.x+1,m_CharSize.y);
	}
///------HEXOFFSETCTRL-----///
void wxHexOffsetCtrl::SetValue( uint64_t position ){
	SetValue( position, BytePerLine );
	}

void wxHexOffsetCtrl::SetValue( uint64_t position, int byteperline ){
	offset_position = position;
	BytePerLine = byteperline;
	m_text.Clear();

    wxString format=GetFormatString();

	wxULongLong_t ull = ( offset_position );
	if( offset_mode == 's' ){//Sector Indicator!
		for( int i=0 ; i<LineCount() ; i++ ){
			m_text << wxString::Format( format, (ull/sector_size), ull%sector_size );
			ull += BytePerLine;
			}
		}
	else
		for( int i=0 ; i<LineCount() ; i++ ){
			m_text << wxString::Format( format, ull );
			ull += BytePerLine;
			}
	RePaint();
	}

wxString wxHexOffsetCtrl::GetFormatedOffsetString( uint64_t c_offset, bool minimal ){
   if(offset_mode=='s')
		return wxString::Format( GetFormatString(minimal), (c_offset/sector_size), c_offset%sector_size );
	return wxString::Format( GetFormatString(minimal), c_offset );
	}

wxString wxHexOffsetCtrl::GetFormatString( bool minimal ){
   wxString format;
   if(offset_mode=='s'){
   	int sector_digit=0;
   	int offset_digit=0;
   	if(!minimal){
			while((1+offset_limit/sector_size) > pow(10,++sector_digit));
			while(sector_size > pow(10,++offset_digit));
			}
		format << wxT("%0") << sector_digit << wxLongLongFmtSpec << wxT(":%0") << offset_digit << wxT("u");
		return format;
		}
	format << wxT("%0") <<
			(minimal? 0 : GetDigitCount())
			<< wxLongLongFmtSpec << wxChar( offset_mode );
   if( offset_mode=='X' )
        format << wxChar('h');
	else if ( offset_mode=='o')
        format << wxChar('o');
	return format;
    }

void wxHexOffsetCtrl::OnMouseRight( wxMouseEvent& event ){
	switch( offset_mode ){
        case 'u': offset_mode = 'X'; break;
        case 'X': offset_mode = 'o'; break;
        case 'o': offset_mode = (sector_size ? 's' :'u'); break;
        case 's': offset_mode = 'u'; break;
        default : offset_mode = 'u';
        }

	wxString s= wxChar( offset_mode );
   myConfigBase::Get()->Write( _T("LastOffsetMode"), s);

	SetValue( offset_position );
	}

void wxHexOffsetCtrl::OnMouseLeft( wxMouseEvent& event ){
	wxPoint p = PixelCoordToInternalCoord( event.GetPosition() );
	uint64_t address = offset_position + p.y*BytePerLine;
	wxString adr;
	if(offset_mode=='s')
		adr = wxString::Format( GetFormatString(), (1+address/sector_size), address%sector_size);
	else
		adr = wxString::Format( GetFormatString(), address);

	if(wxTheClipboard->Open()) {
		wxTheClipboard->Clear();
		if( !wxTheClipboard->SetData( new wxTextDataObject( adr )) )
			wxBell();
		wxTheClipboard->Flush();
		wxTheClipboard->Close();
		}
	}

unsigned wxHexOffsetCtrl::GetDigitCount( void ){
	digit_count=0;
	int base=0;
   switch( offset_mode){
        case 'u': base=10; break;
        case 'X': base=16; break;
        case 'o': base= 8; break;
        case 's': base=10; break;
        }
	if( offset_mode=='s'){
		int digit_count2=0;
		while(1+(offset_limit/sector_size) > pow(base,++digit_count));
		while(sector_size > pow(base,++digit_count2));
		digit_count+=digit_count2;
		}

	while(offset_limit > pow(base,++digit_count));
	if( digit_count < 6)
		digit_count=6;

	return digit_count;
	}

unsigned wxHexOffsetCtrl::GetLineSize( void ){
        unsigned line_size = GetDigitCount();
	    if( offset_mode=='X' || offset_mode=='o' )
            line_size++;
        return line_size;
        }
