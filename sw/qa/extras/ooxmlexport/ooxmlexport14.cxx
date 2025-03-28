/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <swmodeltestbase.hxx>

#include <com/sun/star/awt/FontUnderline.hpp>
#include <com/sun/star/awt/FontWeight.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/style/LineSpacing.hpp>
#include <com/sun/star/style/LineSpacingMode.hpp>
#include <com/sun/star/table/ShadowFormat.hpp>
#include <com/sun/star/text/RelOrientation.hpp>
#include <com/sun/star/text/TableColumnSeparator.hpp>
#include <com/sun/star/text/XDependentTextField.hpp>
#include <com/sun/star/text/XDocumentIndex.hpp>
#include <com/sun/star/text/XDocumentIndexesSupplier.hpp>
#include <com/sun/star/text/XFootnotesSupplier.hpp>
#include <com/sun/star/text/XFootnote.hpp>
#include <com/sun/star/text/XTextContentAppend.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/text/XTextFieldsSupplier.hpp>
#include <com/sun/star/text/XTextGraphicObjectsSupplier.hpp>
#include <com/sun/star/text/XTextTable.hpp>
#include <com/sun/star/text/XTextTablesSupplier.hpp>
#include <com/sun/star/linguistic2/XHyphenator.hpp>
#include <editeng/unolingu.hxx>

#include <comphelper/sequenceashashmap.hxx>
#include <officecfg/Office/Common.hxx>
#include <oox/drawingml/drawingmltypes.hxx>
#include <o3tl/string_view.hxx>

using namespace com::sun::star;

class Test : public SwModelTestBase
{
public:
    Test() : SwModelTestBase(u"/sw/qa/extras/ooxmlexport/data/"_ustr, u"Office Open XML Text"_ustr) {}
};

DECLARE_OOXMLEXPORT_TEST(Tdf130907, "tdf130907.docx")
{
    uno::Reference<text::XTextRange> xPara1 = getParagraph(2);
    CPPUNIT_ASSERT(xPara1.is());
    uno::Reference<beans::XPropertySet> xFormula1Props(xPara1, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xFormula1Props.is());
    sal_Int16 nHOri1;
    xFormula1Props->getPropertyValue(u"ParaAdjust"_ustr) >>= nHOri1;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("The alignment of the equation is not left!",
        sal_Int16(style::ParagraphAdjust::ParagraphAdjust_LEFT), nHOri1);

    uno::Reference<text::XTextRange> xPara2 = getParagraph(3);
    CPPUNIT_ASSERT(xPara2.is());
    uno::Reference<beans::XPropertySet> xFormula2Props(xPara2, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xFormula2Props.is());
    sal_Int16 nHOri2;
    xFormula2Props->getPropertyValue(u"ParaAdjust"_ustr) >>= nHOri2;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("The alignment of the equation is not center!",
        sal_Int16(style::ParagraphAdjust::ParagraphAdjust_CENTER), nHOri2);

    uno::Reference<text::XTextRange> xPara3 = getParagraph(5);
    CPPUNIT_ASSERT(xPara3.is());
    uno::Reference<beans::XPropertySet> xFormula3Props(xPara3, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xFormula3Props.is());
    sal_Int16 nHOri3;
    xFormula3Props->getPropertyValue(u"ParaAdjust"_ustr) >>= nHOri3;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("The alignment of the equation is not right!",
        sal_Int16(style::ParagraphAdjust::ParagraphAdjust_RIGHT), nHOri3);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf128197)
{
    createSwDoc("128197_compat14.docx");
    xmlDocUniquePtr pLayout14 = parseLayoutDump();
    sal_Int32 nHeight14 = getXPath(pLayout14, "//page[1]/body/txt[1]/infos/bounds", "height").toInt32();

    createSwDoc("128197_compat15.docx");
    xmlDocUniquePtr pLayout15 = parseLayoutDump();
    sal_Int32 nHeight15 = getXPath(pLayout15, "//page[1]/body/txt[1]/infos/bounds", "height").toInt32();

    // In compat mode=14 second line has size of the shape thus entire paragraph height is smaller
    // So nHeight14 < nHeight15
    CPPUNIT_ASSERT_LESS(nHeight15, nHeight14);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf135595_HFtableWrap)
{
    loadAndReload("tdf135595_HFtableWrap.odt");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    sal_Int32 nRowHeight = getXPath(pXmlDoc, "//page[1]/header/tab/row/infos/bounds", "height").toInt32();
    // tdf#77794: always force bLayoutInCell from false to true for MSO2013+
    // The fly is supposed to be inside the cell. Before, height was 998. Now it is 2839.
    CPPUNIT_ASSERT_MESSAGE("Image must be contained inside the table cell", nRowHeight > 2000);

    // tdf#162211: wrap-through fly can escape cell in upward direction now
    sal_Int32 nFooterImageBottom
        = getXPath(pXmlDoc,
                   "//page[1]/footer/tab/row/cell[1]/txt/anchored/fly/SwAnchoredObject/bounds",
                   "bottom")
             .toInt32();
    sal_Int32 nFooterBottom
        = getXPath(pXmlDoc, "//page[1]/footer/infos/bounds", "bottom").toInt32();
    // the image is above the bottom of the footer
    CPPUNIT_ASSERT(nFooterBottom > nFooterImageBottom); // image is higher footer
}

DECLARE_OOXMLEXPORT_TEST(testTdf135943_shapeWithText_L0c15,
                         "tdf135943_shapeWithText_LayoutInCell0_compat15.docx")
{
    // With compat15, layoutinCell ought to be ignored/forced to true.
    // HOWEVER, currently only the shape is correctly placed, while its text is un-synced separately.
    // So to prevent this ugly mess, just leave everything together in the historical (wrong) spot.
    xmlDocUniquePtr pDump = parseLayoutDump();
    sal_Int32 nFrameLeft = getXPath(pDump, "//anchored/SwAnchoredDrawObject/bounds", "left").toInt32();
    sal_Int32 nFrameRight = getXPath(pDump, "//anchored/SwAnchoredDrawObject/bounds", "right").toInt32();
    sal_Int32 nTextLeft = getXPath(pDump, "//anchored/fly/infos/bounds", "left").toInt32();
    sal_Int32 nTextRight = getXPath(pDump, "//anchored/fly/infos/bounds", "right").toInt32();
    // The text must be inside of its frame boundaries
    CPPUNIT_ASSERT(nFrameRight >= nTextRight);
    CPPUNIT_ASSERT(nFrameLeft <= nTextLeft);
    // LayoutInCell: The text must fit inside cell A1 //cell[1]/info/bounds/right = 4703
    //CPPUNIT_ASSERT(nTextRight < 4704);

    uno::Reference<beans::XPropertySet> xShapeProperties(getShape(1), uno::UNO_QUERY);
    bool bValue;
    xShapeProperties->getPropertyValue(u"IsFollowingTextFlow"_ustr) >>= bValue;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("YOU FIXED ME? LayoutInCell ought to be true", false, bValue);
}

DECLARE_OOXMLEXPORT_TEST(testTdf135595_HFtableWrap_c12, "tdf135595_HFtableWrap_c12.docx")
{
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    // tdf#104596: ignore wrap exception apparently does not apply if it is not "layout in table cell".
    // Should be only one page. Row height should be two lines at 722, not wrapped to three lines at 998.
    sal_Int32 nRowHeight = getXPath(pXmlDoc, "//header/tab/row/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_MESSAGE("Text must not wrap around header image", nRowHeight < 800);
}

DECLARE_OOXMLEXPORT_TEST(testTdf151704_thinColumnHeight, "tdf151704_thinColumnHeight.docx")
{
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    sal_Int32 nRowHeightT1 = getXPath(
        pXmlDoc, "//page[1]/body/tab[1]/row/cell/tab[1]/row[1]/infos/bounds", "height").toInt32();
    sal_Int32 nRowHeightT2 = getXPath(
        pXmlDoc, "//page[2]/body/tab/row[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Same row height in both tables", nRowHeightT1, nRowHeightT2);
}

DECLARE_OOXMLEXPORT_TEST(testTdf123622, "tdf123622.docx")
{
    uno::Reference<beans::XPropertySet> XPropsRight(getShape(1),uno::UNO_QUERY);
    sal_Int16 nRelativePosR = 0;
    XPropsRight->getPropertyValue(u"HoriOrientRelation"_ustr)>>=nRelativePosR;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Shape inside the margin", sal_Int16(4), nRelativePosR);

    uno::Reference<beans::XPropertySet> XPropsLeft(getShape(2), uno::UNO_QUERY);
    sal_Int16 nRelativePosL = 0;
    XPropsLeft->getPropertyValue(u"HoriOrientRelation"_ustr) >>= nRelativePosL;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Shape inside the margin", sal_Int16(3), nRelativePosL);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf78749)
{
    loadAndReload("tdf78749.docx");
    //Shape lost the background image before, now check if it still has...
    auto xShape = getShape(1);
    uno::Reference<beans::XPropertySet> xShpProps(xShape, uno::UNO_QUERY);
    OUString aPropertyVal;
    xShpProps->getPropertyValue(u"FillBitmapName"_ustr) >>= aPropertyVal;
    CPPUNIT_ASSERT(!aPropertyVal.isEmpty());
}

CPPUNIT_TEST_FIXTURE(Test, testTdf128207)
{
    loadAndSave("tdf128207.docx");
    //There was the charts on each other, because their horizontal and vertical position was 0!
    xmlDocUniquePtr p_XmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(p_XmlDoc);
    assertXPathContent(p_XmlDoc, "/w:document/w:body/w:p[1]/w:r[1]/w:drawing/wp:anchor/wp:positionH/wp:posOffset", u"4445");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf123873)
{
    loadAndSave("tdf123873.docx");
    //OLE Object were overlapped due to missing wrap import
    xmlDocUniquePtr p_XmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(p_XmlDoc);
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p[2]/w:r[2]/w:drawing/wp:anchor/wp:wrapTopAndBottom");
}

CPPUNIT_TEST_FIXTURE(Test, Tdf133065)
{
    loadAndSave("tdf133065.odt");
    CPPUNIT_ASSERT_EQUAL(7, getShapes());
    CPPUNIT_ASSERT_EQUAL(3, getPages());
    auto pxmldoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pxmldoc);
    OUString aVal;

    aVal = getXPath(pxmldoc, "/w:document/w:body/w:p[3]/w:r[2]/w:object/v:shape/w10:wrap", "type");
    CPPUNIT_ASSERT(aVal.indexOf("square") > -1);
    aVal = getXPath(pxmldoc, "/w:document/w:body/w:p[3]/w:r[2]/w:object/v:shape/w10:wrap", "side");
    CPPUNIT_ASSERT(aVal.indexOf("left") > -1);

    aVal = getXPath(pxmldoc, "/w:document/w:body/w:p[8]/w:r[2]/w:object/v:shape/w10:wrap", "type");
    CPPUNIT_ASSERT(aVal.indexOf("square") > -1);
    aVal = getXPath(pxmldoc, "/w:document/w:body/w:p[8]/w:r[2]/w:object/v:shape/w10:wrap", "side");
    CPPUNIT_ASSERT(aVal.indexOf("right") > -1);

    aVal = getXPath(pxmldoc, "/w:document/w:body/w:p[12]/w:r[2]/w:object/v:shape/w10:wrap", "type");
    CPPUNIT_ASSERT(aVal.indexOf("square") > -1);
    aVal = getXPath(pxmldoc, "/w:document/w:body/w:p[12]/w:r[2]/w:object/v:shape/w10:wrap", "side");
    CPPUNIT_ASSERT(aVal.indexOf("largest") > -1);

    aVal = getXPath(pxmldoc, "/w:document/w:body/w:p[20]/w:r[2]/w:object/v:shape/w10:wrap", "type");
    CPPUNIT_ASSERT(aVal.indexOf("topAndBottom") > -1);

    aVal = getXPath(pxmldoc, "/w:document/w:body/w:p[24]/w:r[2]/w:object/v:shape/w10:wrap", "type");
    CPPUNIT_ASSERT(aVal.indexOf("square") > -1);
}

DECLARE_OOXMLEXPORT_TEST(testTdf130814model, "tdf130814.docx")
{
    CPPUNIT_ASSERT_EQUAL(Color(0x1F497D), getProperty<Color>(getRun(getParagraph(2), 1), u"CharColor"_ustr));
    CPPUNIT_ASSERT_EQUAL(double(16), getProperty<double>(getRun(getParagraph(2), 1), u"CharHeight"_ustr));
    CPPUNIT_ASSERT_EQUAL(awt::FontUnderline::SINGLE, getProperty<sal_Int16>(getRun(getParagraph(2), 1), u"CharUnderline"_ustr));
    CPPUNIT_ASSERT_EQUAL(u"Candara"_ustr, getProperty<OUString>(getRun(getParagraph(2), 1), u"CharFontName"_ustr));
    CPPUNIT_ASSERT_EQUAL(u"Arial Unicode MS"_ustr, getProperty<OUString>(getRun(getParagraph(2), 1), u"CharFontNameAsian"_ustr));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf107020)
{
    loadAndSave("tdf107020.docx");
    if (!IsDefaultDPI())
        return;
    xmlDocUniquePtr p_XmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(p_XmlDoc);
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p/w:r/w:drawing/wp:inline/a:graphic/a:graphicData/pic:pic/pic:blipFill/a:srcRect", "l", u"4910");
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p/w:r/w:drawing/wp:inline/a:graphic/a:graphicData/pic:pic/pic:blipFill/a:srcRect", "t", u"27183");
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p/w:r/w:drawing/wp:inline/a:graphic/a:graphicData/pic:pic/pic:blipFill/a:srcRect", "r", u"57638");
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p/w:r/w:drawing/wp:inline/a:graphic/a:graphicData/pic:pic/pic:blipFill/a:srcRect", "b", u"48360");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf130814ooxml)
{
    loadAndSave("tdf130814.docx");
    xmlDocUniquePtr p_XmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(p_XmlDoc);
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p[2]/w:r[1]/w:rPr/w:rFonts", "eastAsia", u"Arial Unicode MS");
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p[2]/w:r[1]/w:rPr/w:rFonts", "ascii", u"Candara");
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p[2]/w:r[1]/w:rPr/w:rFonts", "hAnsi", u"Candara");
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p[2]/w:r[1]/w:rPr/w:color", "val", u"1F497D");
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p[2]/w:r[1]/w:rPr/w:sz", "val", u"32");
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p[2]/w:r[1]/w:rPr/w:szCs", "val", u"32");
    assertXPath(
        p_XmlDoc, "/w:document/w:body/w:p[2]/w:r[1]/w:rPr/w:u", "val", u"single");
}

DECLARE_OOXMLEXPORT_TEST(testTdf129888vml, "tdf129888vml.docx")
{
    //the line shape has anchor in the first cell however it has to
    //be positioned to an another cell. To reach this we must handle
    //the o:allowincell attribute of the shape, and its position has
    //to be calculated from the page frame instead of the table:

    uno::Reference<beans::XPropertySet> xShapeProperties(getShape(1), uno::UNO_QUERY);
    bool bValue;
    xShapeProperties->getPropertyValue(u"IsFollowingTextFlow"_ustr) >>= bValue;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("tdf129888vml The line shape has bad place!",
                                 false, bValue);
}

DECLARE_OOXMLEXPORT_TEST(testTdf129888dml, "tdf129888dml.docx")
{
    //the shape has anchor in the first cell however it has to
    //be positioned to the right side of the page. To reach this we must handle
    //the layoutInCell attribute of the shape, and its position has
    //to be calculated from the page frame instead of the table:

    uno::Reference<beans::XPropertySet> xShapeProperties(getShape(1), uno::UNO_QUERY);
    bool bValue;
    xShapeProperties->getPropertyValue(u"IsFollowingTextFlow"_ustr) >>= bValue;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("tdf129888dml The shape has bad place!",
        false, bValue);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf130120)
{
    loadAndSave("tdf130120.docx");
    // Text for exporting the allowincell attribute:
    xmlDocUniquePtr p_XmlDoc = parseExport(u"word/document.xml"_ustr);
    assertXPath(p_XmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc/w:p/w:r/mc:AlternateContent/"
        "mc:Choice/w:drawing/wp:anchor", "layoutInCell", u"0");
}

CPPUNIT_TEST_FIXTURE(Test, Tdf133030)
{
    loadAndSave("tdf133030.docx");
    auto pExport = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pExport);

    assertXPath(pExport, "/w:document/w:body/w:p[3]/m:oMathPara/m:oMathParaPr/m:jc", "val", u"center");
    assertXPath(pExport, "/w:document/w:body/w:p[5]/m:oMathPara/m:oMathParaPr/m:jc", "val", u"left");
    assertXPath(pExport, "/w:document/w:body/w:p[7]/m:oMathPara/m:oMathParaPr/m:jc", "val", u"right");
}

DECLARE_OOXMLEXPORT_TEST(testTdf87569v, "tdf87569_vml.docx")
{
    //the original tdf87569 sample has vml shapes...
    uno::Reference<beans::XPropertySet> xShapeProperties(getShape(1), uno::UNO_QUERY);
    bool bValue;
    xShapeProperties->getPropertyValue(u"IsFollowingTextFlow"_ustr) >>= bValue;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("tdf87569_vml: The Shape is not in the table!",
                                 true, bValue);
}

DECLARE_OOXMLEXPORT_TEST(testTdf133000_numStyleFormatting, "tdf133000_numStyleFormatting.docx")
{
    // Paragraph style's LeftMargin should not override numbering's Left Margin
    xmlDocUniquePtr pDump = parseLayoutDump();
    assertXPathContent(pDump, "/root/page[1]/body/txt[2]", u"First line");
    const sal_Int32 nLevel1Margin = getXPath(pDump, "//page[1]/body/txt[2]/infos/prtBounds", "left").toInt32();
    assertXPathContent(pDump, "/root/page[1]/body/txt[4]", u"One sublevel");
    const sal_Int32 nLevel2Margin = getXPath(pDump, "//page[1]/body/txt[4]/infos/prtBounds", "left").toInt32();
    CPPUNIT_ASSERT( nLevel1Margin < nLevel2Margin );
}

DECLARE_OOXMLEXPORT_TEST(testTdf134260, "tdf134260.docx")
{
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 0
    // - Actual  : 1270

    auto xNum1Levels
        = getProperty<uno::Reference<container::XIndexAccess>>(getParagraph(1), u"NumberingRules"_ustr);

    CPPUNIT_ASSERT_EQUAL(
        sal_Int32(0),
        comphelper::SequenceAsHashMap(xNum1Levels->getByIndex(0))[u"ListtabStopPosition"_ustr]
            .get<sal_Int32>());

    auto xNum2Levels
        = getProperty<uno::Reference<container::XIndexAccess>>(getParagraph(2), u"NumberingRules"_ustr);

    CPPUNIT_ASSERT_EQUAL(
        sal_Int32(0),
        comphelper::SequenceAsHashMap(xNum2Levels->getByIndex(0))[u"ListtabStopPosition"_ustr]
            .get<sal_Int32>());

    auto xNum3Levels
        = getProperty<uno::Reference<container::XIndexAccess>>(getParagraph(3), u"NumberingRules"_ustr);

    CPPUNIT_ASSERT_EQUAL(
        sal_Int32(0),
        comphelper::SequenceAsHashMap(xNum3Levels->getByIndex(0))[u"ListtabStopPosition"_ustr]
            .get<sal_Int32>());
}

CPPUNIT_TEST_FIXTURE(Test, testArabicZeroNumberingFootnote)
{
    // Create a document, set footnote numbering type to ARABIC_ZERO.
    createSwDoc();
    uno::Reference<text::XFootnotesSupplier> xFootnotesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xFootnoteSettings
        = xFootnotesSupplier->getFootnoteSettings();
    sal_uInt16 nNumberingType = style::NumberingType::ARABIC_ZERO;
    xFootnoteSettings->setPropertyValue(u"NumberingType"_ustr, uno::Any(nNumberingType));

    // Insert a footnote.
    uno::Reference<lang::XMultiServiceFactory> xFactory(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XTextContent> xFootnote(
        xFactory->createInstance(u"com.sun.star.text.Footnote"_ustr), uno::UNO_QUERY);
    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XTextContentAppend> xTextContentAppend(xTextDocument->getText(),
                                                                uno::UNO_QUERY);
    xTextContentAppend->appendTextContent(xFootnote, {});

    saveAndReload(u"Office Open XML Text"_ustr);

    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    // Without the accompanying fix in place, this test would have failed with:
    // XPath '/w:document/w:body/w:sectPr/w:footnotePr/w:numFmt' number of nodes is incorrect
    // because the exporter had no idea what markup to use for ARABIC_ZERO.
    assertXPath(pXmlDoc, "/w:document/w:body/w:sectPr/w:footnotePr/w:numFmt", "val", u"decimalZero");
}

CPPUNIT_TEST_FIXTURE(Test, testChicagoNumberingFootnote)
{
    // Create a document, set footnote numbering type to SYMBOL_CHICAGO.
    createSwDoc();
    uno::Reference<text::XFootnotesSupplier> xFootnotesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xFootnoteSettings
        = xFootnotesSupplier->getFootnoteSettings();
    sal_uInt16 nNumberingType = style::NumberingType::SYMBOL_CHICAGO;
    xFootnoteSettings->setPropertyValue(u"NumberingType"_ustr, uno::Any(nNumberingType));

    // Insert a footnote.
    uno::Reference<lang::XMultiServiceFactory> xFactory(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XTextContent> xFootnote(
        xFactory->createInstance(u"com.sun.star.text.Footnote"_ustr), uno::UNO_QUERY);
    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XTextContentAppend> xTextContentAppend(xTextDocument->getText(),
                                                                uno::UNO_QUERY);
    xTextContentAppend->appendTextContent(xFootnote, {});

    saveAndReload(u"Office Open XML Text"_ustr);

    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    // Without the accompanying fix in place, this test would have failed with:
    // XPath '/w:document/w:body/w:sectPr/w:footnotePr/w:numFmt' number of nodes is incorrect
    // because the exporter had no idea what markup to use for SYMBOL_CHICAGO.
    assertXPath(pXmlDoc, "/w:document/w:body/w:sectPr/w:footnotePr/w:numFmt", "val", u"chicago");
}

CPPUNIT_TEST_FIXTURE(Test, testListNotCountedIndent)
{
    loadAndReload("list_notcounted_indent.fodt");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[1]/w:pPr/w:numPr/w:numId", "val", u"0");
    // wrong: 720
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[1]/w:pPr/w:ind", "start", u"0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[1]/w:pPr/w:ind", "end", u"0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[1]/w:pPr/w:ind", "hanging", u"0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:pPr/w:numPr/w:numId", "val", u"2");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:pPr/w:ind", 0);
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[3]/w:pPr/w:numPr/w:numId", "val", u"0");
    // wrong: 180
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[3]/w:pPr/w:ind", "start", u"720");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[3]/w:pPr/w:ind", "hanging", u"0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[4]/w:pPr/w:numPr/w:numId", "val", u"0");
    // wrong: 720
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[4]/w:pPr/w:ind", "start", u"0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[4]/w:pPr/w:ind", "end", u"0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[4]/w:pPr/w:ind", "hanging", u"0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[5]/w:pPr/w:numPr/w:numId", "val", u"0");
    // wrong: 720
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[5]/w:pPr/w:ind", "start", u"0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[5]/w:pPr/w:ind", "end", u"0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[5]/w:pPr/w:ind", "hanging", u"0");
}

DECLARE_OOXMLEXPORT_TEST(testTdf87569d, "tdf87569_drawingml.docx")
{
    //if the original tdf87569 sample is upgraded it will have drawingml shapes...
    uno::Reference<beans::XPropertySet> xShapeProperties(getShape(1), uno::UNO_QUERY);
    bool bValue;
    xShapeProperties->getPropertyValue(u"IsFollowingTextFlow"_ustr) >>= bValue;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("tdf87569_drawingml: The Shape is not in the table!",
                                 true, bValue);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf130610)
{
    loadAndReload("tdf130610_bold_in_2_styles.ott");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // check character properties
    {
        uno::Reference<beans::XPropertySet> xStyle(
            getStyles(u"CharacterStyles"_ustr)->getByName(u"WollMuxRoemischeZiffer"_ustr),
            uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Bold", awt::FontWeight::BOLD, getProperty<float>(xStyle, u"CharWeight"_ustr));
    }

    // check paragraph properties
    {
        uno::Reference<beans::XPropertySet> xStyle(
            getStyles(u"ParagraphStyles"_ustr)->getByName(u"WollMuxVerfuegungspunkt"_ustr),
            uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Bold", awt::FontWeight::BOLD, getProperty<float>(xStyle, u"CharWeight"_ustr));
    }

    // check inline text properties
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r/w:rPr/w:b");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf121045)
{
    loadAndSave("tdf121045.docx");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:pPr/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:pPr/w:rPr/w:szCs", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[1]/w:fldChar", "fldCharType", u"begin");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[2]/w:instrText", 1);
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[3]/w:fldChar", "fldCharType", u"separate");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[5]/w:fldChar", "fldCharType", u"end");
    // form control keeps its direct formatted font size
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[2]/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[2]/w:rPr/w:szCs", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[3]/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[3]/w:rPr/w:szCs", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[4]/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[4]/w:rPr/w:szCs", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[5]/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:p/w:r[5]/w:rPr/w:szCs", "val", u"20");
}

DECLARE_OOXMLEXPORT_TEST(testTdf78352, "tdf78352.docx")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());

    // Ensure that width of first tab is close to zero (previous value was ~1000 twips)
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    int nWidth = getXPath(pXmlDoc, "/root/page/body/txt[1]/SwParaPortion/SwLineLayout/child::*[@type='PortionType::TabLeft']", "width").toInt32();
    CPPUNIT_ASSERT_LESS(150, nWidth);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf81567)
{
    loadAndReload("tdf81567.odt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    CPPUNIT_ASSERT_EQUAL(2, getShapes());

    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    int nFrameWidth = getXPath(pXmlDoc, "/root/page/body/txt/anchored/fly/infos/bounds", "width").toInt32();
    CPPUNIT_ASSERT_EQUAL(2371, nFrameWidth);

    int nFrameHeight = getXPath(pXmlDoc, "/root/page/body/txt/anchored/fly/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_EQUAL(3520, nFrameHeight);

    int nFrameTop = getXPath(pXmlDoc, "/root/page/body/txt/anchored/fly/infos/bounds", "top").toInt32();
    CPPUNIT_ASSERT_EQUAL(1518, nFrameTop);

    int nImageWidth = getXPath(pXmlDoc, "/root/page/body/txt/anchored/fly/txt/anchored/fly/infos/bounds", "width").toInt32();
    CPPUNIT_ASSERT_EQUAL(2370, nImageWidth);

    int nImageHeight = getXPath(pXmlDoc, "/root/page/body/txt/anchored/fly/txt/anchored/fly/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_EQUAL(1605, nImageHeight);

    // Check the image is at the top of the frame
    // Without the fix in place, this test would have failed with:
    // - Expected: 1638
    // - Actual  : 2236
    int nImageTop = getXPath(pXmlDoc, "/root/page/body/txt/anchored/fly/txt/anchored/fly/infos/bounds", "top").toInt32();
    CPPUNIT_ASSERT_EQUAL(1638, nImageTop);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf92472)
{
    loadAndSave("tdf92472.docx");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:pPr/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:pPr/w:rPr/w:szCs", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[1]/w:fldChar", "fldCharType", u"begin");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[2]/w:instrText", 1);
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[3]/w:fldChar", "fldCharType", u"separate");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[5]/w:fldChar", "fldCharType", u"end");
    // form control keeps its direct formatted font size
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[2]/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[2]/w:rPr/w:szCs", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[3]/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[3]/w:rPr/w:szCs", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[4]/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[4]/w:rPr/w:szCs", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[5]/w:rPr/w:sz", "val", u"20");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:r[5]/w:rPr/w:szCs", "val", u"20");
}

CPPUNIT_TEST_FIXTURE(Test, Tdf133035)
{
    loadAndSave("tdf133035.docx");
    auto pxml = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pxml);
    OUString aXmlVal = getXPath(pxml, "/w:document/w:body/w:p[1]/w:r[1]/w:object/v:shape", "style");
    CPPUNIT_ASSERT(aXmlVal.indexOf("margin-left:186.6pt") > -1);
}

DECLARE_OOXMLEXPORT_TEST(testTdf131260, "tdf131260.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTablesSupplier->getTextTables(),
                                                    uno::UNO_QUERY);
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 3
    // - Actual  : 2
    CPPUNIT_ASSERT_EQUAL(sal_Int32(3), xTables->getCount());
}
DECLARE_OOXMLEXPORT_TEST(testTdf120315, "tdf120315.docx")
{
    // tdf#120315 cells of the second column weren't vertically merged
    // because their horizontal positions are different a little bit
    uno::Reference<text::XTextTablesSupplier> xTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTablesSupplier->getTextTables(),
                                                    uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTextTable(xTables->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<table::XTableRows> xTableRows = xTextTable->getRows();
    CPPUNIT_ASSERT_EQUAL(getProperty<uno::Sequence<text::TableColumnSeparator>>(
                             xTableRows->getByIndex(0), u"TableColumnSeparators"_ustr)[0]
                             .Position,
                         getProperty<uno::Sequence<text::TableColumnSeparator>>(
                             xTableRows->getByIndex(1), u"TableColumnSeparators"_ustr)[2]
                             .Position);
}

DECLARE_OOXMLEXPORT_TEST(testTdf108350_noFontdefaults, "tdf108350_noFontdefaults.docx")
{
    uno::Reference< container::XNameAccess > paragraphStyles = getStyles(u"ParagraphStyles"_ustr);
    uno::Reference< beans::XPropertySet > xStyleProps(paragraphStyles->getByName(u"NoParent"_ustr), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(u"Times New Roman"_ustr, getProperty<OUString>(xStyleProps, u"CharFontName"_ustr));
    //CPPUNIT_ASSERT_EQUAL_MESSAGE("Font size", 10.f, getProperty<float>(xStyleProps, "CharHeight"));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf123116_oversizedRowSplit)
{
    loadAndReload("tdf123116_oversizedRowSplit.odt");
    // Intentionally require a very non-backward-compatible, natural continuation of the table
    // instead of an ugly "page break" like MS Word does (and LO used to do).
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Row splits over 4 pages", 4, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testPageContentBottom, "page-content-bottom.docx")
{
    uno::Reference<beans::XPropertySet> xShape(getShape(1), uno::UNO_QUERY);
    sal_Int16 nExpected = text::RelOrientation::PAGE_PRINT_AREA_BOTTOM;
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 10 (PAGE_PRINT_AREA_BOTTOM)
    // - Actual  : 0 (FRAME)
    // i.e. the bottom-of-body relation was lost.
    CPPUNIT_ASSERT_EQUAL(nExpected, getProperty<sal_Int16>(xShape, u"VertOrientRelation"_ustr));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf129522_removeShadowStyle)
{
    loadAndReload("tdf129522_removeShadowStyle.odt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference< container::XNameAccess > paragraphStyles = getStyles(u"ParagraphStyles"_ustr);
    uno::Reference< beans::XPropertySet > xStyleProps(paragraphStyles->getByName(u"Shadow"_ustr), uno::UNO_QUERY_THROW);
    table::ShadowFormat aShadow = getProperty<table::ShadowFormat>(xStyleProps, u"ParaShadowFormat"_ustr);
    CPPUNIT_ASSERT_EQUAL(table::ShadowLocation_BOTTOM_RIGHT, aShadow.Location);

    // Shadows were inherited regardless of whether the style disabled them.
    xStyleProps.set(paragraphStyles->getByName(u"Shadow-removed"_ustr), uno::UNO_QUERY_THROW);
    aShadow = getProperty<table::ShadowFormat>(xStyleProps, u"ParaShadowFormat"_ustr);
    CPPUNIT_ASSERT_EQUAL(table::ShadowLocation_NONE, aShadow.Location);

    uno::Reference< container::XNameAccess > characterStyles = getStyles(u"CharacterStyles"_ustr);
    xStyleProps.set(characterStyles->getByName(u"CharShadow"_ustr), uno::UNO_QUERY_THROW);
    aShadow = getProperty<table::ShadowFormat>(xStyleProps, u"CharShadowFormat"_ustr);
    CPPUNIT_ASSERT_EQUAL(table::ShadowLocation_BOTTOM_RIGHT, aShadow.Location);

    xStyleProps.set(characterStyles->getByName(u"CharShadow-removed"_ustr), uno::UNO_QUERY_THROW);
    aShadow = getProperty<table::ShadowFormat>(xStyleProps, u"CharShadowFormat"_ustr);
    CPPUNIT_ASSERT_EQUAL(table::ShadowLocation_NONE, aShadow.Location);
}

DECLARE_OOXMLEXPORT_TEST(testTdf130167_spilloverHeaderShape, "testTdf130167_spilloverHeader.docx")
{
    uno::Reference<text::XTextGraphicObjectsSupplier> xTextGraphicObjectsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xNameAccess(
        xTextGraphicObjectsSupplier->getGraphicObjects(), uno::UNO_QUERY);
    // graphics from discarded headers were being added to the text body. Reduced from 5 to 2 shapes overall.
    // CPPUNIT_ASSERT(xNameAccess->getCount() <= 4); -> What about hidden headers?
    CPPUNIT_ASSERT_LESS(sal_Int32(9), xNameAccess->getCount());
}

DECLARE_OOXMLEXPORT_TEST(testTdf124986, "tdf124986.docx")
{
    // Load a document with SET fields, where the SET fields contain leading/trailing quotation marks and spaces.
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    while (xFields->hasMoreElements())
    {
        uno::Reference<lang::XServiceInfo> xServiceInfo(xFields->nextElement(), uno::UNO_QUERY);
        uno::Reference<beans::XPropertySet> xPropertySet(xServiceInfo, uno::UNO_QUERY);
        if (xServiceInfo->supportsService(u"com.sun.star.text.TextField.SetExpression"_ustr))
        {
            OUString aValue;
            xPropertySet->getPropertyValue(u"Content"_ustr) >>= aValue;
            CPPUNIT_ASSERT_EQUAL(u"demo"_ustr, aValue);
        }
    }
}

DECLARE_OOXMLEXPORT_TEST(testTdf83309, "tdf83309.docx")
{
    // Important: bug case had 4 pages
    CPPUNIT_ASSERT_EQUAL(2, getPages());

    // First paragraph does not have tab before
    // (same applies to all paragraphs in doc, but let's assume they are
    // behaving same way)
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    OUString sNodeType = getXPath(pXmlDoc, "(/root/page[1]/body/txt[1]/SwParaPortion/SwLineLayout/child::*)[1]", "type");
    CPPUNIT_ASSERT_EQUAL(u"PortionType::Text"_ustr, sNodeType);

    // tdf148380: creation-date field in header.xml was unsupported on export
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    auto xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());
    uno::Reference<text::XTextField> xField(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(u"8/31/14 10:26 AM"_ustr, xField->getPresentation(false));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf121661)
{
    loadAndSave("tdf121661.docx");
    xmlDocUniquePtr pXmlSettings = parseExport(u"word/settings.xml"_ustr);
    assertXPath(pXmlSettings, "/w:settings/w:hyphenationZone", "val", u"851");

    // tdf#149421
    uno::Reference<beans::XPropertySet> xStyle(getStyles(u"ParagraphStyles"_ustr)->getByName(u"Standard"_ustr), uno::UNO_QUERY);
    // This was false
    CPPUNIT_ASSERT_GREATER( static_cast<sal_Int16>(0), getProperty<sal_Int16>(xStyle, u"ParaHyphenationZone"_ustr));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf149421)
{
    auto verify = [this](bool bIsExport = false) {
        uno::Reference<beans::XPropertySet> xStyle(getStyles(u"ParagraphStyles"_ustr)->getByName(u"Standard"_ustr), uno::UNO_QUERY);
        // This was false
        CPPUNIT_ASSERT_GREATER( static_cast<sal_Int16>(0), getProperty<sal_Int16>(xStyle, u"ParaHyphenationZone"_ustr));

        if (!bIsExport)
        {
            CPPUNIT_ASSERT_EQUAL( static_cast<sal_Int16>(851), getProperty<sal_Int16>(xStyle, u"ParaHyphenationZone"_ustr));
            // modify hyphenation zone (note: only hyphenation zone set in Standard paragraph style
            // is exported, according to the document-level hyphenation settings of OOXML)
            xStyle->setPropertyValue(u"ParaHyphenationZone"_ustr, uno::Any(static_cast<sal_Int16>(2000)));
        }
        else
        {
            // check the export of the modified hyphenation zone
            CPPUNIT_ASSERT_EQUAL( static_cast<sal_Int16>(2000), getProperty<sal_Int16>(xStyle, u"ParaHyphenationZone"_ustr));
        }
    };

    createSwDoc("tdf121661.docx");
    verify();
    saveAndReload(mpFilter);
    verify(/*bIsExport*/ true);
}

DECLARE_OOXMLEXPORT_TEST(testTdf149421_default, "tdf146171.docx")
{
    uno::Reference<beans::XPropertySet> xStyle(getStyles(u"ParagraphStyles"_ustr)->getByName(u"Standard"_ustr), uno::UNO_QUERY);
    // This was 0 (not the default OOXML hyphenationZone)
    CPPUNIT_ASSERT_EQUAL( static_cast<sal_Int16>(360), getProperty<sal_Int16>(xStyle, u"ParaHyphenationZone"_ustr));
}

DECLARE_OOXMLEXPORT_TEST(testTdf161628, "tdf132599_frames_on_right_pages_no_hyphenation.fodt")
{
    uno::Reference<beans::XPropertySet> xStyle(getStyles(u"ParagraphStyles"_ustr)->getByName(u"Standard"_ustr), uno::UNO_QUERY);
    // This was 360 after the second import (missing export of zero hyphenation zone)
    CPPUNIT_ASSERT_EQUAL( static_cast<sal_Int16>(0), getProperty<sal_Int16>(xStyle, u"ParaHyphenationZone"_ustr));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf161643)
{
    loadAndSave("fdo76163.docx");
    xmlDocUniquePtr pXmlSettings = parseExport(u"word/settings.xml"_ustr);
    assertXPath(pXmlSettings, "/w:settings/w:consecutiveHyphenLimit", "val", u"1");

    uno::Reference<beans::XPropertySet> xStyle(getStyles(u"ParagraphStyles"_ustr)->getByName(u"Standard"_ustr), uno::UNO_QUERY);
    // This was false (value 0)
    CPPUNIT_ASSERT_EQUAL( static_cast<sal_Int16>(1), getProperty<sal_Int16>(xStyle, u"ParaHyphenationMaxHyphens"_ustr));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf121658)
{
    loadAndSave("tdf121658.docx");
    xmlDocUniquePtr pXmlSettings = parseExport(u"word/settings.xml"_ustr);
    assertXPath(pXmlSettings, "/w:settings/w:doNotHyphenateCaps");
}

CPPUNIT_TEST_FIXTURE(Test, testTableStyleConfNested)
{
    loadAndSave("table-style-conf-nested.docx");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);
    // Without the accompanying fix in place, this test would have failed, as the custom table cell
    // border properties were lost, so the outer A2 cell started to have borders, not present in the
    // doc model.
    assertXPath(pXmlDoc, "//w:body/w:tbl/w:tr/w:tc[2]/w:tcPr/w:tcBorders/w:top", "val", u"nil");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf133771)
{
    // Create the doc model.
    createSwDoc("tdf133771.odt", /*pPassword*/ "test");

    CPPUNIT_ASSERT_EQUAL(u"Password Protected"_ustr, getParagraph(1)->getString());

    // Without the fix in place, this test would have failed with
    // "An uncaught exception of type com.sun.star.io.IOException"
    // exporting to docx
    save(u"Office Open XML Text"_ustr);
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);
    assertXPathContent(pXmlDoc, "//w:body/w:p/w:r/w:t", u"Password Protected");
}

CPPUNIT_TEST_FIXTURE(Test, testZeroLineSpacing)
{
    // Create the doc model.
    createSwDoc();
    uno::Reference<beans::XPropertySet> xParagraph(getParagraph(1), uno::UNO_QUERY);
    style::LineSpacing aSpacing;
    aSpacing.Mode = style::LineSpacingMode::MINIMUM;
    aSpacing.Height = 0;
    xParagraph->setPropertyValue(u"ParaLineSpacing"_ustr, uno::Any(aSpacing));

    // Export to docx.
    save(u"Office Open XML Text"_ustr);
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);

    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: atLeast
    // - Actual  : auto
    // i.e. the minimal linespacing was lost on export.
    assertXPath(pXmlDoc, "/w:document/w:body/w:p/w:pPr/w:spacing", "lineRule", u"atLeast");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p/w:pPr/w:spacing", "line", u"0");
}

CPPUNIT_TEST_FIXTURE(Test, testSemiTransparentText)
{
    // Create an in-memory empty document.
    createSwDoc();

    // Set text to half-transparent and type a character.
    uno::Reference<beans::XPropertySet> xParagraph(getParagraph(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xParagraph.is());
    sal_Int16 nTransparence = 75;
    xParagraph->setPropertyValue(u"CharTransparence"_ustr, uno::Any(nTransparence));
    uno::Reference<text::XTextRange> xTextRange(xParagraph, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xTextRange.is());
    xTextRange->setString(u"x"_ustr);

    // Export to docx.
    save(u"Office Open XML Text"_ustr);
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);
    double fValue = getXPath(
            pXmlDoc,
            "/w:document/w:body/w:p/w:r/w:rPr/w14:textFill/w14:solidFill/w14:srgbClr/w14:alpha",
            "val")
        .toDouble();
    sal_Int16 nActual = basegfx::fround(fValue / oox::drawingml::PER_PERCENT);

    // Without the accompanying fix in place, this test would have failed, as the w14:textFill
    // element was missing.
    CPPUNIT_ASSERT_EQUAL(nTransparence, nActual);
}

DECLARE_OOXMLEXPORT_TEST(testTdf147485, "Tdf147485.docx")
{
    // Before the fix this was impossible.
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf149546, "tdf149546.docx")
{
    // Before the fix this was impossible.
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

CPPUNIT_TEST_FIXTURE(Test, testUserField)
{
    // Create an in-memory empty document with a user field.
    createSwDoc();
    uno::Reference<lang::XMultiServiceFactory> xFactory(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XDependentTextField> xField(
        xFactory->createInstance(u"com.sun.star.text.TextField.User"_ustr), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xMaster(
        xFactory->createInstance(u"com.sun.star.text.FieldMaster.User"_ustr), uno::UNO_QUERY);
    xMaster->setPropertyValue(u"Name"_ustr, uno::Any(u"foo"_ustr));
    xField->attachTextFieldMaster(xMaster);
    xField->getTextFieldMaster()->setPropertyValue(u"Content"_ustr, uno::Any(u"bar"_ustr));
    uno::Reference<text::XTextDocument> xDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XText> xText = xDocument->getText();
    xText->insertTextContent(xText->createTextCursor(), xField, /*bAbsorb=*/false);

    // Export to docx.
    save(u"Office Open XML Text"_ustr);
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);

    // Without the accompanying fix in place, this test would have failed, the user field was
    // exported as <w:t>User Field foo = bar</w:t>.
    assertXPathContent(pXmlDoc, "//w:p/w:r[2]/w:instrText", u" DOCVARIABLE foo ");
    assertXPathContent(pXmlDoc, "//w:p/w:r[4]/w:t", u"bar");

    // Make sure that not only the variables, but also their values are written.
    pXmlDoc = parseExport(u"word/settings.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);
    assertXPath(pXmlDoc, "//w:docVars/w:docVar", "name", u"foo");
    assertXPath(pXmlDoc, "//w:docVars/w:docVar", "val", u"bar");
}

CPPUNIT_TEST_FIXTURE(Test, testHighlightEdit_numbering)
{
    // Create the doc model.
    createSwDoc("tdf135774_numberingCRProps.docx");

    // This only affects when saving as w:highlight - which is not the default since 7.0.
    bool bWasExportToShade = !officecfg::Office::Common::Filter::Microsoft::Export::CharBackgroundToHighlighting::get();
    auto batch = comphelper::ConfigurationChanges::create();
    officecfg::Office::Common::Filter::Microsoft::Export::CharBackgroundToHighlighting::set(true, batch);
    batch->commit();

    //Simulate a user editing the char background color of the paragraph 2 marker (CR)
    uno::Reference<beans::XPropertySet> properties(getParagraph(2), uno::UNO_QUERY);
    uno::Sequence<beans::NamedValue> aListAutoFormat;
    CPPUNIT_ASSERT(properties->getPropertyValue(u"ListAutoFormat"_ustr) >>= aListAutoFormat);
    comphelper::SequenceAsHashMap aMap(properties->getPropertyValue(u"ListAutoFormat"_ustr));
    // change the background color to RES_CHRATR_BACKGROUND.
    aMap[u"CharBackColor"_ustr] <<= static_cast<sal_Int32>(0xff00ff);
    // Two attributes can affect character background. Highlight has priority, and is only there for MS compatibility,
    // so clear any potential highlight set earlier, or override any coming via a style.
    aMap[u"CharHighlight"_ustr] <<= static_cast<sal_Int32>(COL_TRANSPARENT);

    uno::Sequence<beans::PropertyValue> aGrabBag;
    aMap[u"CharInteropGrabBag"_ustr] >>= aGrabBag;
    for (beans::PropertyValue& rProp : asNonConstRange(aGrabBag))
    {
        // The shading is no longer defined from import, so clear that flag.
        // BackColor 0xff00ff will now attempt to export as highlight, since we set that in officecfg.
        if (rProp.Name == "CharShadingMarker")
            rProp.Value <<= false;
    }
    aMap[u"CharInteropGrabBag"_ustr] <<= aGrabBag;

    aMap >> aListAutoFormat;
    properties->setPropertyValue(u"ListAutoFormat"_ustr, uno::Any(aListAutoFormat));

    // Export to docx.
    save(u"Office Open XML Text"_ustr);

    // Paragraph 2 should have only one w:highlight written per w:rPr. Without the fix, there were two.
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    assertXPath(pXmlDoc, "//w:body/w:p[2]/w:pPr/w:rPr/w:highlight", "val", u"none");
    // Visually, the "none" highlight means the bullet point should not have a character background.

    xmlDocUniquePtr pXmlStyles = parseExport(u"word/styles.xml"_ustr);
    assertXPath(pXmlStyles, "//w:style[@w:styleId='CustomParaStyleHighlightGreen']/w:rPr/w:highlight", "val", u"green");
    // Visually, the last bullet point's text should be green-highlighted (but the bullet point itself shouldn't)

    if (bWasExportToShade)
    {
        officecfg::Office::Common::Filter::Microsoft::Export::CharBackgroundToHighlighting::set(false, batch);
        batch->commit();
    }
}

CPPUNIT_TEST_FIXTURE(Test, testTdf132766)
{
    loadAndSave("tdf132766.docx");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/numbering.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);

    // Ensure that for list=1 and level=0 we wrote correct bullet char and correct font
    assertXPath(pXmlDoc, "//w:numbering/w:abstractNum[@w:abstractNumId='1']/w:lvl[@w:ilvl='0']/w:lvlText",
                "val", u"\uF0B7");
    assertXPath(pXmlDoc, "//w:numbering/w:abstractNum[@w:abstractNumId='1']/w:lvl[@w:ilvl='0']/w:rPr/w:rFonts",
                "ascii", u"Symbol");
    assertXPath(pXmlDoc, "//w:numbering/w:abstractNum[@w:abstractNumId='1']/w:lvl[@w:ilvl='0']/w:rPr/w:rFonts",
                "hAnsi", u"Symbol");
    assertXPath(pXmlDoc, "//w:numbering/w:abstractNum[@w:abstractNumId='1']/w:lvl[@w:ilvl='0']/w:rPr/w:rFonts",
                "cs", u"Symbol");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf128245)
{
    loadAndSave("tdf128245.docx");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/numbering.xml"_ustr);
    CPPUNIT_ASSERT(pXmlDoc);

    assertXPath(pXmlDoc, "//w:num[@w:numId='1']/w:abstractNumId", "val", u"1");
    assertXPath(pXmlDoc, "//w:num[@w:numId='2']/w:abstractNumId", "val", u"2");
    assertXPath(pXmlDoc, "//w:num[@w:numId='3']/w:abstractNumId", "val", u"3");
    assertXPath(pXmlDoc, "//w:num[@w:numId='4']/w:abstractNumId", "val", u"1");
    assertXPath(pXmlDoc, "//w:num[@w:numId='4']/w:lvlOverride[@w:ilvl='0']", "ilvl", u"0");
    assertXPath(pXmlDoc, "//w:num[@w:numId='4']/w:lvlOverride/w:startOverride", "val", u"1");
    assertXPath(pXmlDoc, "//w:num[@w:numId='4']/w:lvlOverride[@w:ilvl='1']", 0);
}

DECLARE_OOXMLEXPORT_TEST(testTdf124367, "tdf124367.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTablesSupplier->getTextTables(),
                                                    uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTextTable(xTables->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<table::XTableRows> xTableRows = xTextTable->getRows();
    // it was 2761 at the first import, and 2760 at the second import, due to incorrect rounding
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(2762),
                         getProperty<uno::Sequence<text::TableColumnSeparator>>(
                             xTableRows->getByIndex(2), u"TableColumnSeparators"_ustr)[0]
                             .Position);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf95189)
{
    loadAndReload("tdf95189.docx");
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(1), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"1"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(2), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"2"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(3), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"1"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(4), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"2"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(5), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"3"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(6), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"1"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(7), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"2"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
}

CPPUNIT_TEST_FIXTURE(Test, testTdf128820)
{
    loadAndSave("tdf128820.fodt");
    // Import of exported DOCX failed because of wrong namespace used for wsp element
    // Now test the exported XML, in case we stop failing opening invalid files
    xmlDocUniquePtr pXml = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    // The parent wpg:wgp element has three children: wpg:cNvGrpSpPr, wpg:grpSpPr, and wpg:wsp
    // (if we start legitimately exporting additional children, this needs to be adjusted to check
    // all those, to make sure we don't export wrong elements).
    assertXPathChildren(pXml,
                        "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/"
                        "wp:inline/a:graphic/a:graphicData/wpg:wgp",
                        3);
    assertXPath(pXml,
                "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:inline/"
                "a:graphic/a:graphicData/wpg:wgp/wpg:cNvGrpSpPr");
    assertXPath(pXml,
                "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:inline/"
                "a:graphic/a:graphicData/wpg:wgp/wpg:grpSpPr");
    // This one was pic:wsp instead of wps:wsp
    assertXPath(pXml,
                "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:inline/"
                "a:graphic/a:graphicData/wpg:wgp/wps:wsp");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf128889)
{
    loadAndSave("tdf128889.fodt");
    xmlDocUniquePtr pXml = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    // There was an w:r (with w:br) as an invalid child of first paragraph's w:pPr
    assertXPath(pXml, "/w:document/w:body/w:p[1]/w:pPr/w:r", 0);
    assertXPath(pXml, "/w:document/w:body/w:p[1]/w:r", 2);
    // Check that the break is in proper - last - position
    assertXPath(pXml, "/w:document/w:body/w:p[1]/w:r[2]/w:br", "type", u"page");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf132754)
{
    loadAndReload("tdf132754.docx");
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(1), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"0.0.0."_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(2), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"0.0.1."_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(3), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(u"0.0.2."_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
}

DECLARE_OOXMLEXPORT_TEST(testTdf129353, "tdf129353.docx")
{
    CPPUNIT_ASSERT_EQUAL(8, getParagraphs());
    getParagraph(1, u"(Verne, 1870)"_ustr);
    getParagraph(2, u"Bibliography"_ustr);
    getParagraph(4, u"Christie, A. (1922). The Secret Adversary. "_ustr);
    CPPUNIT_ASSERT_EQUAL(OUString(), getParagraph(8)->getString());

    uno::Reference<text::XDocumentIndexesSupplier> xIndexSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xIndexes = xIndexSupplier->getDocumentIndexes();
    uno::Reference<text::XDocumentIndex> xIndex(xIndexes->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<text::XTextRange> xTextRange = xIndex->getAnchor();
    uno::Reference<text::XText> xText = xTextRange->getText();
    uno::Reference<text::XTextCursor> xTextCursor = xText->createTextCursor();
    xTextCursor->gotoRange(xTextRange->getStart(), false);
    xTextCursor->gotoRange(xTextRange->getEnd(), true);
    OUString aIndexString(convertLineEnd(xTextCursor->getString(), LineEnd::LINEEND_LF));

    // Check that all the pre-rendered entries are correct, including trailing spaces
    CPPUNIT_ASSERT_EQUAL(u"\n" // starting with an empty paragraph
                                  "Christie, A. (1922). The Secret Adversary. \n"
                                  "\n"
                                  "Verne, J. G. (1870). Twenty Thousand Leagues Under the Sea. \n"
                                  ""_ustr, // ending with an empty paragraph
                         aIndexString);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf77796)
{
    loadAndSave("tdf77796.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    // cell paddings from table style
    assertXPath(pXml, "/w:document/w:body/w:tbl/w:tblPr/w:tblCellMar/w:start", "w", u"5");
    assertXPath(pXml, "/w:document/w:body/w:tbl/w:tblPr/w:tblCellMar/w:top", "w", u"240");
    assertXPath(pXml, "/w:document/w:body/w:tbl/w:tblPr/w:tblCellMar/w:bottom", "w", u"480");
    // not modified
    assertXPath(pXml, "/w:document/w:body/w:tbl/w:tblPr/w:tblCellMar/w:end", "w", u"108");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf128290)
{
    loadAndSave("tdf128290.odt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    xmlDocUniquePtr pXml = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    assertXPath(pXml, "/w:document/w:body/w:tbl/w:tblPr/w:tblLayout", "type", u"fixed");

    // ensure unnecessary suppressLineNumbers entry is not created.
    xmlDocUniquePtr pStylesXml = parseExport(u"word/styles.xml"_ustr);
    assertXPath(pStylesXml, "//w:style[@w:styleId='Normal']/w:pPr/w:suppressLineNumbers", 0);
}

DECLARE_OOXMLEXPORT_TEST(testTdf120394, "tdf120394.docx")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(1), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(0), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(u"1"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(2), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(1), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(OUString(), getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(3), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(1), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(OUString(), getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(5), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(2), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(u"1.2.1"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
}

DECLARE_OOXMLEXPORT_TEST(testTdf133605, "tdf133605.docx")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(3), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(0), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(OUString(), getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(4), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(1), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(u"1."_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(5), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(2), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(u"1.1"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(6), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(3), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(u"(a)"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
}

DECLARE_OOXMLEXPORT_TEST(testTdf133605_2, "tdf133605_2.docx")
{
    // About the same document as tdf133605.docx, but number definition has level definitions in random order
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(3), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(0), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(OUString(), getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(4), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(1), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(u"1."_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(5), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(2), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(u"1.1"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(6), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(3), getProperty<sal_Int16>(xPara, u"NumberingLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(u"(a)"_ustr, getProperty<OUString>(xPara, u"ListLabelString"_ustr));
    }
}

CPPUNIT_TEST_FIXTURE(Test, testTdf123757)
{
    loadAndSave("tdf123757.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    assertXPath(pXml, "/w:document/w:body/w:tbl", 2);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf141172)
{
    loadAndSave("tdf141172.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/endnotes.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    // This was 1 (lost table during copying endnote content)
    assertXPath(pXml, "/w:endnotes/w:endnote/w:tbl", 2);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf141548)
{
    loadAndSave("tdf141548.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/endnotes.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    // This was 0 (lost text content of the run with endnoteRef)
    assertXPath(pXml, "/w:endnotes/w:endnote[4]/w:p/w:r[2]/w:t", 2);
    assertXPathContent(pXml, "/w:endnotes/w:endnote[4]/w:p/w:r[2]/w:t[1]", u"another endnote");
    assertXPathContent(pXml, "/w:endnotes/w:endnote[4]/w:p/w:r[2]/w:t[2]", u"new line");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf143399)
{
    loadAndSave("tdf143399.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/footnotes.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    // These were 0 (lost text content of documents both with footnotes and endnotes)
    assertXPath(pXml, "/w:footnotes/w:footnote[3]/w:p/w:r[3]/w:t", 1);
    assertXPathContent(pXml, "/w:footnotes/w:footnote[3]/w:p/w:r[3]/w:t", u"Footnotes_graphic2");
    assertXPath(pXml, "/w:footnotes/w:footnote[4]/w:p/w:r[3]/w:t", 1);
    assertXPathContent(pXml, "/w:footnotes/w:footnote[4]/w:p/w:r[3]/w:t", u"Footnotes_graphic");

    xmlDocUniquePtr pXml2 = parseExport(u"word/endnotes.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    // This was 0 (lost text content of the run with endnoteRef)
    assertXPath(pXml2, "/w:endnotes/w:endnote[3]/w:p/w:r[3]/w:t", 1);
    assertXPathContent(pXml2, "/w:endnotes/w:endnote[3]/w:p/w:r[3]/w:t[1]", u"Endnotes");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf143583)
{
    loadAndSave("tdf143583_emptyParaAtEndOfFootnote.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/footnotes.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    assertXPath(pXml, "/w:footnotes/w:footnote[3]/w:p", 2);
    // This was 1
    assertXPath(pXml, "/w:footnotes/w:footnote[4]/w:p", 2);
    // This was 2
    assertXPath(pXml, "/w:footnotes/w:footnote[5]/w:p", 3);
    // This was 2
    assertXPath(pXml, "/w:footnotes/w:footnote[6]/w:p", 3);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf152203)
{
    loadAndSave("tdf152203.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/footnotes.xml"_ustr);
    CPPUNIT_ASSERT(pXml);

    uno::Reference<text::XFootnotesSupplier> xFootnotesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xFootnotes = xFootnotesSupplier->getFootnotes();
    uno::Reference<text::XTextRange> xLastFootnote(xFootnotes->getByIndex(5), uno::UNO_QUERY);
    // This was "Footnote for pg5" (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg 6"_ustr, xLastFootnote->getString().trim() );

    uno::Reference<text::XTextRange> xLastButOne(xFootnotes->getByIndex(4), uno::UNO_QUERY);
    // This was "Footnote for pg 6" (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg5"_ustr, xLastButOne->getString().trim() );
}

CPPUNIT_TEST_FIXTURE(Test, testTdf152506)
{
    loadAndSave("tdf152506.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/footnotes.xml"_ustr);
    CPPUNIT_ASSERT(pXml);

    uno::Reference<text::XFootnotesSupplier> xFootnotesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xFootnotes = xFootnotesSupplier->getFootnotes();
    uno::Reference<text::XTextRange> xLastFootnote(xFootnotes->getByIndex(1), uno::UNO_QUERY);
    // This was "Footnote for pg5" (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg 6"_ustr, xLastFootnote->getString().trim() );

    uno::Reference<text::XTextRange> xLastButOne(xFootnotes->getByIndex(0), uno::UNO_QUERY);
    // This was "Footnote for pg 6" (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg5"_ustr, xLastButOne->getString().trim() );
}

CPPUNIT_TEST_FIXTURE(Test, testTdf153255)
{
    loadAndSave("tdf153255.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/footnotes.xml"_ustr);
    CPPUNIT_ASSERT(pXml);

    uno::Reference<text::XFootnotesSupplier> xFootnotesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xFootnotes = xFootnotesSupplier->getFootnotes();
    uno::Reference<text::XTextRange> xLastFootnote(xFootnotes->getByIndex(5), uno::UNO_QUERY);
    // This was "Footnote for pg2" (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg 6"_ustr, xLastFootnote->getString().trim() );

    uno::Reference<text::XTextRange> xLastButOne(xFootnotes->getByIndex(4), uno::UNO_QUERY);
    // This was "Footnote for pg 6" (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg5"_ustr, xLastButOne->getString().trim() );

    // check all the remaining footnotes

    uno::Reference<text::XTextRange> xFootnote1(xFootnotes->getByIndex(0), uno::UNO_QUERY);
    // This was "Footnote for pg3" (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg1"_ustr, xFootnote1->getString().trim() );

    uno::Reference<text::XTextRange> xFootnote2(xFootnotes->getByIndex(1), uno::UNO_QUERY);
    // This was "Footnote for pg5" (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg2"_ustr, xFootnote2->getString().trim() );

    uno::Reference<text::XTextRange> xFootnote3(xFootnotes->getByIndex(2), uno::UNO_QUERY);
    // This was "Footnote for pg4." (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg3"_ustr, xFootnote3->getString().trim() );

    uno::Reference<text::XTextRange> xFootnote4(xFootnotes->getByIndex(3), uno::UNO_QUERY);
    // This was "Footnote for pg1" (replaced footnotes)
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg4."_ustr, xFootnote4->getString().trim() );
}

CPPUNIT_TEST_FIXTURE(Test, testTdf153804)
{
    loadAndSave("tdf153804.docx");
    xmlDocUniquePtr pXml = parseExport(u"word/footnotes.xml"_ustr);
    CPPUNIT_ASSERT(pXml);

    uno::Reference<text::XFootnotesSupplier> xFootnotesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xFootnotes = xFootnotesSupplier->getFootnotes();
    uno::Reference<text::XTextRange> xLastFootnote(xFootnotes->getByIndex(1), uno::UNO_QUERY);
    // This was empty
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg 6"_ustr, xLastFootnote->getString().trim() );

    uno::Reference<text::XTextRange> xLastButOne(xFootnotes->getByIndex(0), uno::UNO_QUERY);
    // This was empty
    CPPUNIT_ASSERT_EQUAL( u"Footnote for pg5"_ustr, xLastButOne->getString().trim() );
}

// skip test for macOS (missing fonts?)
#if !defined(MACOSX)
DECLARE_OOXMLEXPORT_TEST(testTdf146346, "tdf146346.docx")
{
    // This was 2 (by bad docDefault vertical margins around tables in footnotes)
    CPPUNIT_ASSERT_EQUAL(1, getPages());

    // only first page has table
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();

    // check first page: all tables on the first page
    assertXPath(pXmlDoc, "/root/page[1]//anchored/fly", 8);
    assertXPath(pXmlDoc, "/root/page[1]//anchored/fly/tab", 8);

    // No second page.
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 0
    // - Actual  : 1
    // i.e. unwanted lower margin in the floating table's anchor paragraph in the footnote created a
    // second page.
    assertXPath(pXmlDoc, "/root/page[2]", 0);
}
#endif

DECLARE_OOXMLEXPORT_TEST(testTdf130088, "tdf130088.docx")
{
    // This was 2 (justification without shrinking resulted more lines)
    CPPUNIT_ASSERT_EQUAL(1, getPages());

    // check compatibility option in ODT export/import, too
    saveAndReload(u"writer8"_ustr);

    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf158436, "tdf158436.docx")
{
    // This resulted freezing
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf159032, "tdf159032.docx")
{
    // This resulted crashing
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf160518, "tdf160518_useWord2013TrackBottomHyphenation.docx")
{
    uno::Reference<linguistic2::XHyphenator> xHyphenator = LinguMgr::GetHyphenator();
    if (!xHyphenator->hasLocale(lang::Locale(u"en"_ustr, u"US"_ustr, OUString())))
        return;

    // This was 2 (without shifting last hyphenated line of the page)
    CPPUNIT_ASSERT_EQUAL(3, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf160518_compatible, "tdf160518_allowHyphenationAtTrackBottom.docx")
{
    uno::Reference<linguistic2::XHyphenator> xHyphenator = LinguMgr::GetHyphenator();
    if (!xHyphenator->hasLocale(lang::Locale(u"en"_ustr, u"US"_ustr, OUString())))
        return;

    // This is still 2
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf160518_ODT, "tdf160518_useWord2013TrackBottomHyphenation.docx")
{
    uno::Reference<linguistic2::XHyphenator> xHyphenator = LinguMgr::GetHyphenator();
    if (!xHyphenator->hasLocale(lang::Locale(u"en"_ustr, u"US"_ustr, OUString())))
        return;

    // This was 2 (without shifting last hyphenated line of the page)
    CPPUNIT_ASSERT_EQUAL(3, getPages());

    // check compatibility option in ODT export/import, too
    saveAndReload(u"writer8"_ustr);

    CPPUNIT_ASSERT_EQUAL(3, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf160518_ODT_compatible, "tdf160518_allowHyphenationAtTrackBottom.docx")
{
    uno::Reference<linguistic2::XHyphenator> xHyphenator = LinguMgr::GetHyphenator();
    if (!xHyphenator->hasLocale(lang::Locale(u"en"_ustr, u"US"_ustr, OUString())))
        return;

    // This is still 2
    CPPUNIT_ASSERT_EQUAL(2, getPages());

    // check compatibility option in ODT export/import, too
    saveAndReload(u"writer8"_ustr);

    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

CPPUNIT_TEST_FIXTURE(Test, testTdf160518_page_in_default_paragraph_style)
{
    // default paragraph style contains hyphenation settings
    loadAndReload("tdf160518_page_in_default_paragraph_style.fodt");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/settings.xml"_ustr);
    assertXPath(pXmlDoc, "/w:settings/w:compat/w:compatSetting[@w:name='useWord2013TrackBottomHyphenation']", "val", u"1");
    assertXPath(pXmlDoc, "/w:settings/w:compat/w:compatSetting[@w:name='allowHyphenationAtTrackBottom']", 0);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf160518_auto_in_default_paragraph_style)
{
    // default paragraph style contains hyphenation settings
    loadAndReload("tdf160518_auto_in_default_paragraph_style.fodt");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/settings.xml"_ustr);
    assertXPath(pXmlDoc, "/w:settings/w:compat/w:compatSetting[@w:name='useWord2013TrackBottomHyphenation']", "val", u"1");
    assertXPath(pXmlDoc, "/w:settings/w:compat/w:compatSetting[@w:name='allowHyphenationAtTrackBottom']", "val", u"1");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf160518_page_in_text_body_style)
{
    // text body style contains hyphenation settings
    loadAndReload("tdf160518_page_in_text_body_style.fodt");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/settings.xml"_ustr);
    assertXPath(pXmlDoc, "/w:settings/w:compat/w:compatSetting[@w:name='useWord2013TrackBottomHyphenation']", "val", u"1");
    assertXPath(pXmlDoc, "/w:settings/w:compat/w:compatSetting[@w:name='allowHyphenationAtTrackBottom']", 0);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf160518_auto_in_text_body_style)
{
    // text body style contains hyphenation settings
    loadAndReload("tdf160518_auto_in_text_body_style.fodt");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/settings.xml"_ustr);
    assertXPath(pXmlDoc, "/w:settings/w:compat/w:compatSetting[@w:name='useWord2013TrackBottomHyphenation']", "val", u"1");
    assertXPath(pXmlDoc, "/w:settings/w:compat/w:compatSetting[@w:name='allowHyphenationAtTrackBottom']", "val", u"1");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf165354)
{
    uno::Reference<linguistic2::XHyphenator> xHyphenator = LinguMgr::GetHyphenator();
    if (!xHyphenator->hasLocale(lang::Locale(u"en"_ustr, u"US"_ustr, OUString())))
        return;

    loadAndReload("tdf165354.docx");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    // This was "except that it has an at" (hyphenation at the end of the page)
    assertXPath(pXmlDoc, "//page[1]/body/txt[2]/SwParaPortion/SwLineLayout[9]", "portion", u"except that it has an ");
    // This started with "mosphere" (hyphenation at the end of the previous page)
    assertXPath(pXmlDoc, "//page[2]/body/txt[1]/SwParaPortion/SwLineLayout[1]", "portion", u"atmosphere. The Earth ");
    // The same word is still hyphenated in the same paragraph, but not at the bottom of the page
    assertXPath(pXmlDoc, "//page[2]/body/txt[1]/SwParaPortion/SwLineLayout[9]", "portion", u"except that it has an at");
}

CPPUNIT_TEST_FIXTURE(Test, testHyphenationAuto)
{
    loadAndReload("hyphenation.odt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Explicitly set hyphenation=auto on document level
    xmlDocUniquePtr pXmlSettings = parseExport(u"word/settings.xml"_ustr);
    CPPUNIT_ASSERT(pXmlSettings);
    assertXPath(pXmlSettings, "/w:settings/w:autoHyphenation", "val", u"true");

    // Second paragraph has explicitly enabled hyphenation
    xmlDocUniquePtr pXml = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(pXml);
    assertXPath(pXml, "/w:document/w:body/w:p[2]/w:pPr/w:suppressAutoHyphens", "val", u"false");

    // Default paragraph style explicitly disables hyphens
    xmlDocUniquePtr pXmlStyles = parseExport(u"word/styles.xml"_ustr);
    CPPUNIT_ASSERT(pXmlStyles);
    assertXPath(pXmlStyles, "/w:styles/w:docDefaults/w:pPrDefault/w:pPr/w:suppressAutoHyphens", "val", u"true");
}

CPPUNIT_TEST_FIXTURE(Test, testStrikeoutGroupShapeText)
{
    loadAndSave("tdf131776_StrikeoutGroupShapeText.docx");
    // tdf#131776: Check if strikeout is used in shape group texts
    xmlDocUniquePtr pXml = parseExport(u"word/document.xml"_ustr);

    // double strike (dstrike)
    //   no "val" attribute
    assertXPath(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[1]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:dstrike");
    assertXPathNoAttribute(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[1]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:dstrike", "val");
    //   "val" attribute is true, this is used in this test file. However, LO is going to export this element without the "val" attribute
    //   because if the element appears, but without an attribute its "val" true by default.
    assertXPath(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[2]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:dstrike");
    assertXPathNoAttribute(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[2]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:dstrike", "val");
    //   "val" attribute is false (this was missing, resulting the regression)
    assertXPath(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[3]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:dstrike", "val", u"false");

    // simple strike (strike)
        //   no "val" attribute
    assertXPath(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[4]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:strike");
    assertXPathNoAttribute(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[4]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:strike", "val");
    //   "val" attribute is true, this is used in this test file. However, LO is going to export this element without the "val" attribute
    //   because if the element appears, but without an attribute its "val" true by default.
    assertXPath(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[5]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:strike");
    assertXPathNoAttribute(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[5]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:strike", "val");
    //   "val" attribute is false
    assertXPath(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wpg:wgp/"
        "wps:wsp[6]/wps:txbx/w:txbxContent/w:p/w:r/w:rPr/w:strike", "val", u"false");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf131539)
{
    loadAndSave("tdf131539.odt");
    CPPUNIT_ASSERT_EQUAL(2, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    //The positions of OLE objects were not exported, check if now it is exported correctly
    xmlDocUniquePtr p_XmlDoc = parseExport(u"word/document.xml"_ustr);
    CPPUNIT_ASSERT(p_XmlDoc);
    OUString aXmlVal = getXPath(p_XmlDoc, "/w:document/w:body/w:p[4]/w:r[1]/w:object/v:shape", "style");
    // This data was missing
    CPPUNIT_ASSERT(aXmlVal.indexOf("margin-left:139.95")>-1);
}

CPPUNIT_TEST_FIXTURE(Test, testLineWidthRounding)
{
    loadAndSave("tdf126363_LineWidthRounding.docx");
    // tdf#126363: check if line with stays the same after export
    xmlDocUniquePtr pXml = parseExport(u"word/document.xml"_ustr);
    // this was 57240 (it differs from the original 57150, losing the preset line width)
    assertXPath(pXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/a:graphic/a:graphicData/wps:wsp/wps:spPr/a:ln", "w", u"57150");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf108505)
{
    loadAndReload("tdf108505.docx");
    uno::Reference<text::XTextRange> xParagraph = getParagraph(3);
    uno::Reference<text::XTextRange> xText
        = getRun(xParagraph, 1, u"Wrong font when alone on the line"_ustr);

    // Without the fix in place this would have become Times New Roman
    CPPUNIT_ASSERT_EQUAL(
        u"Trebuchet MS"_ustr,
        getProperty<OUString>(xText, u"CharFontName"_ustr));
}

CPPUNIT_TEST_FIXTURE(Test, testRelativeAnchorHeightFromTopMarginHasHeader)
{
    loadAndReload("tdf123324_testRelativeAnchorHeightFromTopMarginHasHeader.docx");
    // tdf#123324 The height was set relative to page print area top,
    // but this was handled relative to page height.
    // Note: page print area top = margin + header height.
    // In this case the header exists.
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    assertXPath(pXmlDoc, "//anchored/SwAnchoredDrawObject/bounds", "height", u"2551");
}

CPPUNIT_TEST_FIXTURE(Test, testRelativeAnchorHeightFromTopMarginNoHeader)
{
    loadAndReload("tdf123324_testRelativeAnchorHeightFromTopMarginNoHeader.docx");
    // tdf#123324 The height was set relative from top margin, but this was handled relative from page height.
    // Note: the MSO Word margin = LO margin + LO header height.
    // In this case the header does not exist, so MSO Word margin and LO Writer margin are the same.

    // tdf#123324 The height was set relative to page print area top,
    // but this was handled relative to page height.
    // Note: page print area top = margin + header height.
    // In this case the header does not exist, so OpenDocument and OOXML margins are the same.
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    assertXPath(pXmlDoc, "//anchored/SwAnchoredDrawObject/bounds", "height", u"2551");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf64531)
{
    loadAndReload("tdf64531.docx");
    xmlDocUniquePtr pXmlDoc= parseExport(u"word/document.xml"_ustr);
    OString sPathToTabs= "/w:document/w:body/w:sdt/w:sdtContent/w:p[2]/w:pPr/w:tabs/"_ostr;
    assertXPath(pXmlDoc, sPathToTabs+"w:tab[1]", "pos",u"720");
    assertXPath(pXmlDoc, sPathToTabs+"w:tab[2]", "pos",u"12950");
}
/* temporarily disabled to get further test results
   The import now uses the dml shape, not the VML fallback.
DECLARE_OOXMLEXPORT_TEST(testVmlShapeTextWordWrap, "tdf97618_testVmlShapeTextWordWrap.docx")
{
    // tdf#97618 The text wrapping of a shape was not handled in a canvas.
    // TODO: fix export too
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    if (!pXmlDoc)
        return;
    // The bound rect of shape will be wider if wrap does not work (the wrong value is 3167).
    assertXPath(pXmlDoc, "//anchored/SwAnchoredDrawObject/bounds", "width", "2500");
}
*/

CPPUNIT_TEST_FIXTURE(Test, testVmlLineShapeMirroredX)
{
    // tdf#97517 The "flip:x" was not handled for VML line shapes.
    loadAndSave("tdf97517_testVmlLineShapeMirroredX.docx");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    OUString sStyle = getXPath(pXmlDoc,
        "/w:document/w:body/w:p[3]/w:r/mc:AlternateContent/mc:Fallback/w:pict/v:line",
        "style");
    CPPUNIT_ASSERT(sStyle.indexOf("flip:x") > 0);
}

CPPUNIT_TEST_FIXTURE(Test, testVmlLineShapeMirroredY)
{
    // tdf#137678 The "flip:y" was not handled for VML line shapes.
    loadAndSave("tdf137678_testVmlLineShapeMirroredY.docx");
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    OUString sStyle = getXPath(pXmlDoc,
        "/w:document/w:body/w:p[3]/w:r/mc:AlternateContent/mc:Fallback/w:pict/v:line",
        "style");
    CPPUNIT_ASSERT(sStyle.indexOf("flip:y") > 0);
}

CPPUNIT_TEST_FIXTURE(Test, testVmlLineShapeRotated)
{
    loadAndSave("tdf137765_testVmlLineShapeRotated.docx");
    // tdf#137765 The "rotation" (in style attribute) was not handled correctly for VML line shapes.
    xmlDocUniquePtr pXmlDoc = parseExport(u"word/document.xml"_ustr);
    // it was 1.55pt,279.5pt
    assertXPath(pXmlDoc,
        "/w:document/w:body/w:p[3]/w:r/mc:AlternateContent/mc:Fallback/w:pict/v:line",
        "from",
        u"-9pt,296.75pt");
    // it was 25.5pt,317.8pt
    assertXPath(pXmlDoc,
        "/w:document/w:body/w:p[3]/w:r/mc:AlternateContent/mc:Fallback/w:pict/v:line",
        "to",
        u"36.05pt,300.55pt");
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
