#include <QtTest>
#include "model/Document.h"

class TestSketch : public QObject
{
    Q_OBJECT

private slots:
    void testIsSketchType();
    void testBuildSketchRectangle();
    void testBuildSketchCircle();
    void testBuildSketchEllipse();
    void testBuildSketchPolygon();
    void testBuildSketchTriangle();
    void testBuildSketchOnDifferentPlanes();
    void testBuildSketchWithOffset();
};

void TestSketch::testIsSketchType()
{
    QVERIFY(Document::isSketchType("Sketch_Rectangle"));
    QVERIFY(Document::isSketchType("Sketch_Circle"));
    QVERIFY(!Document::isSketchType("Box"));
    QVERIFY(!Document::isSketchType("Cylinder"));
}

void TestSketch::testBuildSketchRectangle()
{
    QMap<QString, double> params;
    params["Width"] = 100.0;
    params["Height"] = 60.0;

    TopoDS_Shape face = Document::rebuildSketch("Sketch_Rectangle", params, 0, 0.0);
    QVERIFY(!face.IsNull());
    QCOMPARE(face.ShapeType(), TopAbs_FACE);
}

void TestSketch::testBuildSketchCircle()
{
    QMap<QString, double> params;
    params["Radius"] = 50.0;

    TopoDS_Shape face = Document::rebuildSketch("Sketch_Circle", params, 0, 0.0);
    QVERIFY(!face.IsNull());
    QCOMPARE(face.ShapeType(), TopAbs_FACE);
}

void TestSketch::testBuildSketchEllipse()
{
    QMap<QString, double> params;
    params["Major Radius"] = 60.0;
    params["Minor Radius"] = 30.0;

    TopoDS_Shape face = Document::rebuildSketch("Sketch_Ellipse", params, 0, 0.0);
    QVERIFY(!face.IsNull());
    QCOMPARE(face.ShapeType(), TopAbs_FACE);
}

void TestSketch::testBuildSketchPolygon()
{
    QMap<QString, double> params;
    params["Radius"] = 40.0;
    params["Sides"] = 6.0;

    TopoDS_Shape face = Document::rebuildSketch("Sketch_Polygon", params, 0, 0.0);
    QVERIFY(!face.IsNull());
    QCOMPARE(face.ShapeType(), TopAbs_FACE);
}

void TestSketch::testBuildSketchTriangle()
{
    QMap<QString, double> params;
    params["Side"] = 80.0;

    TopoDS_Shape face = Document::rebuildSketch("Sketch_Triangle", params, 0, 0.0);
    QVERIFY(!face.IsNull());
    QCOMPARE(face.ShapeType(), TopAbs_FACE);
}

void TestSketch::testBuildSketchOnDifferentPlanes()
{
    QMap<QString, double> params;
    params["Radius"] = 30.0;

    // XY plane
    TopoDS_Shape xy = Document::rebuildSketch("Sketch_Circle", params, 0, 0.0);
    QVERIFY(!xy.IsNull());

    // XZ plane
    TopoDS_Shape xz = Document::rebuildSketch("Sketch_Circle", params, 1, 0.0);
    QVERIFY(!xz.IsNull());

    // YZ plane
    TopoDS_Shape yz = Document::rebuildSketch("Sketch_Circle", params, 2, 0.0);
    QVERIFY(!yz.IsNull());
}

void TestSketch::testBuildSketchWithOffset()
{
    QMap<QString, double> params;
    params["Width"] = 50.0;
    params["Height"] = 50.0;

    TopoDS_Shape face = Document::rebuildSketch("Sketch_Rectangle", params, 0, 25.0);
    QVERIFY(!face.IsNull());
    QCOMPARE(face.ShapeType(), TopAbs_FACE);
}

QTEST_MAIN(TestSketch)
#include "tst_Sketch.moc"
