
/****************************************************************
   This file was created automatically by `FlexCat 2.6.2'
   from "Catalogs_Src/FlexCat.cd".

   Do NOT edit by hand!
****************************************************************/

#ifndef FlexCat_CAT_H
#define FlexCat_CAT_H


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#include <libraries/gadtools.h>

void            LocalizeNewMenu ( struct NewMenu *nm );
void            LocalizeStringArray ( STRPTR * Array );
void            OpenFlexCatCatalog (  );
void            CloseFlexCatCatalog (  );

struct FC_String
{
    STRPTR          Str;
    const LONG      Id;
};

extern struct FC_String FlexCat_Strings[];

#define msgMemoryError (FlexCat_Strings[0].Str)
#define _msgMemoryError 0
#define msgWarning (FlexCat_Strings[1].Str)
#define _msgWarning 1
#define msgExpectedHex (FlexCat_Strings[2].Str)
#define _msgExpectedHex 2
#define msgExpectedOctal (FlexCat_Strings[3].Str)
#define _msgExpectedOctal 3
#define msgNoCatalogDescription (FlexCat_Strings[4].Str)
#define _msgNoCatalogDescription 4
#define msgNoLengthBytes (FlexCat_Strings[5].Str)
#define _msgNoLengthBytes 5
#define msgUnknownCDCommand (FlexCat_Strings[6].Str)
#define _msgUnknownCDCommand 6
#define msgUnexpectedBlanks (FlexCat_Strings[7].Str)
#define _msgUnexpectedBlanks 7
#define msgNoIdentifier (FlexCat_Strings[8].Str)
#define _msgNoIdentifier 8
#define msgNoLeadingBracket (FlexCat_Strings[9].Str)
#define _msgNoLeadingBracket 9
#define msgDoubleID (FlexCat_Strings[10].Str)
#define _msgDoubleID 10
#define msgDoubleIdentifier (FlexCat_Strings[11].Str)
#define _msgDoubleIdentifier 11
#define msgNoMinLen (FlexCat_Strings[12].Str)
#define _msgNoMinLen 12
#define msgNoMaxLen (FlexCat_Strings[13].Str)
#define _msgNoMaxLen 13
#define msgNoTrailingBracket (FlexCat_Strings[14].Str)
#define _msgNoTrailingBracket 14
#define msgExtraCharacters (FlexCat_Strings[15].Str)
#define _msgExtraCharacters 15
#define msgNoString (FlexCat_Strings[16].Str)
#define _msgNoString 16
#define msgShortString (FlexCat_Strings[17].Str)
#define _msgShortString 17
#define msgLongString (FlexCat_Strings[18].Str)
#define _msgLongString 18
#define msgNoCatalogTranslation (FlexCat_Strings[19].Str)
#define _msgNoCatalogTranslation 19
#define msgNoCTCommand (FlexCat_Strings[20].Str)
#define _msgNoCTCommand 20
#define msgUnknownCTCommand (FlexCat_Strings[21].Str)
#define _msgUnknownCTCommand 21
#define msgNoCTVersion (FlexCat_Strings[22].Str)
#define _msgNoCTVersion 22
#define msgNoCTLanguage (FlexCat_Strings[23].Str)
#define _msgNoCTLanguage 23
#define msgNoCatalog (FlexCat_Strings[24].Str)
#define _msgNoCatalog 24
#define msgNoNewCTFile (FlexCat_Strings[25].Str)
#define _msgNoNewCTFile 25
#define msgUnknownIdentifier (FlexCat_Strings[26].Str)
#define _msgUnknownIdentifier 26
#define msgNoSourceDescription (FlexCat_Strings[27].Str)
#define _msgNoSourceDescription 27
#define msgNoSource (FlexCat_Strings[28].Str)
#define _msgNoSource 28
#define msgUnknownStringType (FlexCat_Strings[29].Str)
#define _msgUnknownStringType 29
#define msgNoTerminateBracket (FlexCat_Strings[30].Str)
#define _msgNoTerminateBracket 30
#define msgUsage_1 (FlexCat_Strings[31].Str)
#define _msgUsage_1 31
#define msgUsage_2 (FlexCat_Strings[32].Str)
#define _msgUsage_2 32
#define msgUsage_3 (FlexCat_Strings[33].Str)
#define _msgUsage_3 33
#define msgUsage_4 (FlexCat_Strings[34].Str)
#define _msgUsage_4 34
#define msgUsage_5 (FlexCat_Strings[35].Str)
#define _msgUsage_5 35
#define msgUsage_6 (FlexCat_Strings[36].Str)
#define _msgUsage_6 36
#define msgUsage_7 (FlexCat_Strings[37].Str)
#define _msgUsage_7 37
#define msgUsage_8 (FlexCat_Strings[38].Str)
#define _msgUsage_8 38
#define msgUsage_9 (FlexCat_Strings[39].Str)
#define _msgUsage_9 39
#define msgUsage_10 (FlexCat_Strings[40].Str)
#define _msgUsage_10 40
#define msgUsage_11 (FlexCat_Strings[41].Str)
#define _msgUsage_11 41
#define msgNoCTArgument (FlexCat_Strings[42].Str)
#define _msgNoCTArgument 42
#define msgNoBinChars (FlexCat_Strings[43].Str)
#define _msgNoBinChars 43
#define msgCTGap (FlexCat_Strings[44].Str)
#define _msgCTGap 44
#define msgDoubleCTLanguage (FlexCat_Strings[45].Str)
#define _msgDoubleCTLanguage 45
#define msgDoubleCTVersion (FlexCat_Strings[46].Str)
#define _msgDoubleCTVersion 46
#define msgWrongRcsId (FlexCat_Strings[47].Str)
#define _msgWrongRcsId 47
#define msgUsageHead (FlexCat_Strings[48].Str)
#define _msgUsageHead 48
#define msgPrefsError (FlexCat_Strings[49].Str)
#define _msgPrefsError 49
#define msgUsage_12 (FlexCat_Strings[50].Str)
#define _msgUsage_12 50
#define msgUsage_13 (FlexCat_Strings[51].Str)
#define _msgUsage_13 51
#define msgUsage_14 (FlexCat_Strings[52].Str)
#define _msgUsage_14 52
#define msgUpToDate (FlexCat_Strings[53].Str)
#define _msgUpToDate 53
#define msgCantCheckDate (FlexCat_Strings[54].Str)
#define _msgCantCheckDate 54
#define msgUsage_15 (FlexCat_Strings[55].Str)
#define _msgUsage_15 55
#define msgUsage_16 (FlexCat_Strings[56].Str)
#define _msgUsage_16 56
#define msgTrailingEllipsis (FlexCat_Strings[57].Str)
#define _msgTrailingEllipsis 57
#define msgTrailingSpaces (FlexCat_Strings[58].Str)
#define _msgTrailingSpaces 58
#define msgUsage_17 (FlexCat_Strings[59].Str)
#define _msgUsage_17 59
#define msgUsage_18 (FlexCat_Strings[60].Str)
#define _msgUsage_18 60
#define msgNoCTFileName (FlexCat_Strings[61].Str)
#define _msgNoCTFileName 61
#define msgNoCatFileName (FlexCat_Strings[62].Str)
#define _msgNoCatFileName 62

#endif
