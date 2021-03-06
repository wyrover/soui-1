//////////////////////////////////////////////////////////////////////////
//  Class Name: SFontPool
// Description: Font Pool
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "souistd.h"
#include "res.mgr/sfontpool.h"
#include "helper/SplitString.h"

namespace SOUI
{

template<> SFontPool* SSingleton<SFontPool>::ms_Singleton    = 0;

SFontPool::SFontPool(IRenderFactory *pRendFactory)
    :m_RenderFactory(pRendFactory)
{
    m_pFunOnKeyRemoved=OnKeyRemoved;
}


IFontPtr SFontPool::GetFont(FONTSTYLE style, const SStringT & fontFaceName,pugi::xml_node xmlExProp)
{
	IFontPtr hftRet=0;

	SStringT strFace = fontFaceName;
	if(strFace.IsEmpty()) strFace = GetDefFontInfo().strFaceName;
	
	pugi::xml_writer_buff writer;
	xmlExProp.print(writer,L"\t",pugi::format_default,pugi::encoding_utf16);
	SStringT strXmlProp= S_CW2T(SStringW(writer.buffer(),writer.size()));

	FontInfo info = {style.dwStyle,strFace,strXmlProp};

	if(HasKey(info))
	{
		hftRet=GetKeyObject(info);
	}
	else
	{
		hftRet = _CreateFont(info.dwStyle,info.strFaceName,xmlExProp);
		AddKeyObject(info,hftRet);
	}
	return hftRet;

}

static const TCHAR  KFontPropSeprator=  _T(',');   //字体属性之间的分隔符，不再支持其它符号。
static const TCHAR  KPropSeprator    =  _T(':');   //一个属性name:value对之间的分隔符
static const TCHAR  KAttrFalse[]     =   _T("0");
static const TCHAR  KFontFace[]      =   _T("face");
static const TCHAR  KFontBold[]      =   _T("bold");
static const TCHAR  KFontUnderline[] =   _T("underline");
static const TCHAR  KFontItalic[]    =   _T("italic");
static const TCHAR  KFontStrike[]    =   _T("strike");
static const TCHAR  KFontAdding[]    =   _T("adding");
static const TCHAR  KFontSize[]      =   _T("size");
static const TCHAR  KFontCharset[]   =   _T("charset");


#define LEN_FACE    (ARRAYSIZE(KFontFace)-1)
#define LEN_BOLD    (ARRAYSIZE(KFontBold)-1)
#define LEN_UNDERLINE    (ARRAYSIZE(KFontUnderline)-1)
#define LEN_ITALIC  (ARRAYSIZE(KFontItalic)-1)
#define LEN_STRIKE  (ARRAYSIZE(KFontStrike)-1)
#define LEN_ADDING  (ARRAYSIZE(KFontAdding)-1)
#define LEN_SIZE    (ARRAYSIZE(KFontSize)-1)
#define LEN_CHARSET (ARRAYSIZE(KFontCharset)-1)

IFontPtr SFontPool::GetFont( const SStringW & strFont )
{
    FONTSTYLE fntStyle(GetDefFontInfo().dwStyle);
	fntStyle.cSize = 0;

    SStringT strFace;//不需要默认字体, 在后面GetFont里会检查.

    SStringT attr=S_CW2T(strFont);                           
    attr.MakeLower();                                        
    SStringTList fontProp;
    SplitString(attr,KFontPropSeprator,fontProp);

	short cAdding = 0;
	short cSize = 0;

	pugi::xml_document docExProp;
	pugi::xml_node nodePropEx = docExProp.append_child(L"propex");
    for(int i=fontProp.GetCount()-1;i>=0;i--)
    {
        SStringTList strPair;
        if(2!=SplitString(fontProp[i],KPropSeprator,strPair))
        {
            fontProp.RemoveAt(i);
            continue;
        }
        if(strPair[0] == KFontFace)
        {
            strFace = strPair[1];
        }else if(strPair[0] == KFontAdding)
        {
            cAdding=(short)_ttoi(strPair[1]); 
        }else if(strPair[0] == KFontSize)
        {
            cSize=(short)_ttoi(strPair[1]); 
        }else if(strPair[0] == KFontItalic)
        {
            fntStyle.fItalic = strPair[1] != KAttrFalse;
        }else if(strPair[0] == KFontBold)
        {
            fntStyle.fBold = strPair[1] != KAttrFalse;
        }else if(strPair[0] == KFontStrike)
        {
            fntStyle.fStrike = strPair[1] != KAttrFalse;
        }else if(strPair[0] == KFontUnderline)
        {
            fntStyle.fUnderline = strPair[1] != KAttrFalse;
        }else if(strPair[0] == KFontCharset)
        {
            fntStyle.byCharset = (BYTE)_ttoi(strPair[1]);
        }else
		{
			nodePropEx.append_attribute(S_CT2W(strPair[0])).set_value(S_CT2W(strPair[1]));
		}
    }

	if(cSize!=0)
	{//size property is higher than adding
		fntStyle.cSize = cSize;
	}
	else
	{
		FONTSTYLE fontStyle(GetDefFontInfo().dwStyle);
		fntStyle.cSize = fontStyle.cSize + cAdding;  //cAdding为正代表字体变大，否则变小
	}
    return GetFont(fntStyle, strFace,nodePropEx);
}


IFontPtr SFontPool::_CreateFont(const LOGFONT &lf)
{
    
    SASSERT(m_RenderFactory);
    
    
    IFontPtr pFont=NULL;
    m_RenderFactory->CreateFont(&pFont,lf);

    return pFont;
}

IFontPtr SFontPool::_CreateFont(FONTSTYLE style,const SStringT & strFaceName,pugi::xml_node xmlExProp)
{
	LOGFONT lfNew={0};
        
	lfNew.lfCharSet     = style.byCharset;
    lfNew.lfWeight      = (style.fBold ? FW_BOLD : FW_NORMAL);
    lfNew.lfUnderline   = (FALSE != style.fUnderline);
    lfNew.lfItalic      = (FALSE != style.fItalic);
    lfNew.lfStrikeOut   = (FALSE != style.fStrike);
	lfNew.lfHeight = -abs((short)style.cSize);
        
    lfNew.lfQuality = CLEARTYPE_QUALITY;
    
    
    _tcscpy_s(lfNew.lfFaceName,_countof(lfNew.lfFaceName),  strFaceName);
    
    IFontPtr ret = _CreateFont(lfNew);
	if(ret) ret->InitFromXml(xmlExProp);
	return ret;
}

const FontInfo & SFontPool::GetDefFontInfo() const
{
	return SUiDef::getSingleton().GetUiDef()->GetDefFontInfo();
}


}//namespace SOUI