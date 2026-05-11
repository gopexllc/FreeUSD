package freeusd

import (
	"encoding/binary"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestVersionString(t *testing.T) {
	v := VersionString()
	if v == "" {
		t.Fatal("expected non-empty version")
	}
}

func TestUsdcCrateIdentifier(t *testing.T) {
	if UsdcCrateIdentifier() != "PXR-USDC" {
		t.Fatalf("unexpected crate id %q", UsdcCrateIdentifier())
	}
}

func TestDetectUsdFileKindFromPath(t *testing.T) {
	fixture := filepath.Join("..", "..", "tests", "fixtures", "stage_open_root.usda")
	k, d, rc := DetectUsdFileKindFromPath(fixture)
	if rc != 0 {
		t.Fatalf("fixture usda rc=%d %s", rc, LastErrorMessage())
	}
	if d != "" {
		t.Fatalf("unexpected detail %q", d)
	}
	if k != UsdFileKindUsdaAscii {
		t.Fatalf("expected USDA kind, got %v", k)
	}

	dir := t.TempDir()
	cratePath := filepath.Join(dir, "smoke.usdc")
	payload := append([]byte(UsdcCrateIdentifier()), 0)
	if err := os.WriteFile(cratePath, payload, 0o644); err != nil {
		t.Fatal(err)
	}
	k2, _, rc2 := DetectUsdFileKindFromPath(cratePath)
	if rc2 != 0 {
		t.Fatalf("crate rc=%d %s", rc2, LastErrorMessage())
	}
	if k2 != UsdFileKindUsdcCrate {
		t.Fatalf("expected crate kind, got %v", k2)
	}

	missing := filepath.Join(dir, "no_such_file.usda")
	k3, d3, rc3 := DetectUsdFileKindFromPath(missing)
	if rc3 != 0 {
		t.Fatalf("missing file rc=%d %s", rc3, LastErrorMessage())
	}
	if k3 != UsdFileKindIoOrEmpty {
		t.Fatalf("expected IoOrEmpty for missing path, got %v", k3)
	}
	if d3 == "" {
		t.Fatal("expected non-empty detail for missing file")
	}
}

func TestReadUsdcBootstrapFromPath(t *testing.T) {
	dir := t.TempDir()
	p := filepath.Join(dir, "boot.usdc")
	buf := make([]byte, 128)
	copy(buf[0:], UsdcCrateIdentifier())
	buf[8] = 0
	buf[9] = 8
	buf[10] = 0
	binary.LittleEndian.PutUint64(buf[16:24], uint64(88))
	if err := os.WriteFile(p, buf, 0o644); err != nil {
		t.Fatal(err)
	}
	b, rc := ReadUsdcBootstrapFromPath(p)
	if rc != 0 {
		t.Fatalf("bootstrap rc=%d %s", rc, LastErrorMessage())
	}
	if b.FileVersionMajor != 0 || b.FileVersionMinor != 8 || b.FileVersionPatch != 0 || b.TocByteOffset != 88 {
		t.Fatalf("unexpected bootstrap %+v", b)
	}
	_, rcBad := ReadUsdcBootstrapFromPath(filepath.Join(dir, "missing.usdc"))
	if rcBad == 0 {
		t.Fatal("expected error for missing path")
	}
}

func TestReadUsdcTocFromPath(t *testing.T) {
	dir := t.TempDir()
	p := filepath.Join(dir, "toc.usdc")
	buf := make([]byte, 160)
	copy(buf[0:], UsdcCrateIdentifier())
	buf[8] = 0
	buf[9] = 8
	buf[10] = 0
	binary.LittleEndian.PutUint64(buf[16:24], uint64(88))
	binary.LittleEndian.PutUint64(buf[88:96], 2)
	copy(buf[96:103], []byte("TOKENS\x00"))
	copy(buf[128:134], []byte("PATHS\x00"))
	binary.LittleEndian.PutUint64(buf[128+16:128+24], uint64(120))
	binary.LittleEndian.PutUint64(buf[128+24:128+32], uint64(40))
	if err := os.WriteFile(p, buf, 0o644); err != nil {
		t.Fatal(err)
	}
	secs, total, rc := ReadUsdcTocFromPath(p, 16)
	if rc != 0 {
		t.Fatalf("toc rc=%d %s", rc, LastErrorMessage())
	}
	if total != 2 || len(secs) != 2 {
		t.Fatalf("unexpected total=%d len=%d", total, len(secs))
	}
	if secs[0].Name != "TOKENS" || secs[1].Name != "PATHS" || secs[1].StartByteOffset != 120 || secs[1].SizeBytes != 40 {
		t.Fatalf("unexpected sections %+v", secs)
	}
	_, _, rcLow := ReadUsdcTocFromPath(p, 1)
	if rcLow == 0 {
		t.Fatal("expected error when max_sections below file count")
	}
}

func TestLayerStageReadDouble(t *testing.T) {
	const usda = `#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        double x = 3.0
    }
}
`
	l := NewAnonymousLayer("go_smoke")
	if l == nil {
		t.Fatal("NewAnonymousLayer failed:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA rc=%d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("AttachRootLayer failed:", LastErrorMessage())
	}
	defer st.Free()
	v, rc := st.ReadFieldDouble("/W/C", "x", 1.0)
	if rc != 0 {
		t.Fatalf("ReadFieldDouble rc=%d %s", rc, LastErrorMessage())
	}
	if v != 3.0 {
		t.Fatalf("got %v want 3", v)
	}
}

func TestLayerStageReadBoolInt64String(t *testing.T) {
	const usda = `#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        bool flag = true
        int n = 42
        string label = "hi"
    }
}
`
	l := NewAnonymousLayer("go_scalar_str")
	if l == nil {
		t.Fatal("NewAnonymousLayer failed:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA rc=%d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("AttachRootLayer failed:", LastErrorMessage())
	}
	defer st.Free()
	b, rc := st.ReadFieldBool("/W/C", "flag", 1.0)
	if rc != 0 || !b {
		t.Fatalf("ReadFieldBool rc=%d b=%v %s", rc, b, LastErrorMessage())
	}
	n, rc := st.ReadFieldInt64("/W/C", "n", 1.0)
	if rc != 0 || n != 42 {
		t.Fatalf("ReadFieldInt64 rc=%d n=%d %s", rc, n, LastErrorMessage())
	}
	s, rc := st.ReadFieldString("/W/C", "label", 1.0)
	if rc != 0 || s != "hi" {
		t.Fatalf("ReadFieldString rc=%d %q %s", rc, s, LastErrorMessage())
	}
}

func TestLayerStageReadFloat(t *testing.T) {
	const usda = `#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        float r = 1.25
    }
}
`
	l := NewAnonymousLayer("go_float")
	if l == nil {
		t.Fatal("NewAnonymousLayer failed:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA rc=%d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("AttachRootLayer failed:", LastErrorMessage())
	}
	defer st.Free()
	f, rc := st.ReadFieldFloat("/W/C", "r", 1.0)
	if rc != 0 {
		t.Fatalf("ReadFieldFloat rc=%d %s", rc, LastErrorMessage())
	}
	if f != 1.25 {
		t.Fatalf("got %v want 1.25", f)
	}
}

func TestLayerStageReadVec3f(t *testing.T) {
	const usda = `#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        color3f displayColor = (0.25, 0.5, 0.75)
    }
}
`
	l := NewAnonymousLayer("go_vec3f")
	if l == nil {
		t.Fatal("NewAnonymousLayer failed:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA rc=%d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("AttachRootLayer failed:", LastErrorMessage())
	}
	defer st.Free()
	x, y, z, rc := st.ReadFieldVec3f("/W/C", "displayColor", 1.0)
	if rc != 0 {
		t.Fatalf("ReadFieldVec3f rc=%d %s", rc, LastErrorMessage())
	}
	if x != 0.25 || y != 0.5 || z != 0.75 {
		t.Fatalf("got (%v,%v,%v) want (0.25,0.5,0.75)", x, y, z)
	}
}

func TestLayerStageReadVec3d(t *testing.T) {
	const usda = `#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        double3 extent = (1.5, 2.5, 3.5)
    }
}
`
	l := NewAnonymousLayer("go_vec3d")
	if l == nil {
		t.Fatal("NewAnonymousLayer failed:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA rc=%d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("AttachRootLayer failed:", LastErrorMessage())
	}
	defer st.Free()
	x, y, z, rc := st.ReadFieldVec3d("/W/C", "extent", 1.0)
	if rc != 0 {
		t.Fatalf("ReadFieldVec3d rc=%d %s", rc, LastErrorMessage())
	}
	if x != 1.5 || y != 2.5 || z != 3.5 {
		t.Fatalf("got (%v,%v,%v) want (1.5,2.5,3.5)", x, y, z)
	}
}

func TestLayerStageReadMatrix4dQuatToken(t *testing.T) {
	const usda = `#usda 1.0
(
)
def Xform "W"
{
    def "P"
    {
        matrix4d m = ((1,0,0,0), (0,1,0,0), (0,0,1,0), (0,0,0,1))
        quatd qd = (1, 0, 0, 0)
        quatf qf = (0.70710677, 0, 0, 0.70710677)
        token kind = component
        token[] tags = [@a@, @b@]
    }
}
`
	l := NewAnonymousLayer("go_gfreads")
	if l == nil {
		t.Fatal("NewAnonymousLayer failed:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA rc=%d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("AttachRootLayer failed:", LastErrorMessage())
	}
	defer st.Free()
	m, rc := st.ReadFieldMatrix4d("/W/P", "m", 1.0)
	if rc != 0 {
		t.Fatalf("ReadFieldMatrix4d rc=%d %s", rc, LastErrorMessage())
	}
	for i := 0; i < 16; i++ {
		want := 0.0
		if i == 0 || i == 5 || i == 10 || i == 15 {
			want = 1.0
		}
		if m[i] != want {
			t.Fatalf("m[%d]=%v want %v", i, m[i], want)
		}
	}
	qr, qi, qj, qk, rc := st.ReadFieldQuatd("/W/P", "qd", 1.0)
	if rc != 0 || qr != 1.0 || qi != 0 || qj != 0 || qk != 0 {
		t.Fatalf("ReadFieldQuatd rc=%d (%v,%v,%v,%v) %s", rc, qr, qi, qj, qk, LastErrorMessage())
	}
	fr, fi, fj, fk, rc := st.ReadFieldQuatf("/W/P", "qf", 1.0)
	if rc != 0 {
		t.Fatalf("ReadFieldQuatf rc=%d %s", rc, LastErrorMessage())
	}
	if fr < 0.707 || fr > 0.708 || fi != 0 || fj != 0 || fk < 0.707 || fk > 0.708 {
		t.Fatalf("quatf got (%v,%v,%v,%v)", fr, fi, fj, fk)
	}
	tok, rc := st.ReadFieldToken("/W/P", "kind", 1.0)
	if rc != 0 || tok != "component" {
		t.Fatalf("ReadFieldToken rc=%d %q %s", rc, tok, LastErrorMessage())
	}
	tags, rc := st.ReadFieldTokenArray("/W/P", "tags", 1.0)
	if rc != 0 || len(tags) != 2 || tags[0] != "a" || tags[1] != "b" {
		t.Fatalf("ReadFieldTokenArray rc=%d %#v %s", rc, tags, LastErrorMessage())
	}
}

func TestOpenStageFromRootFile(t *testing.T) {
	root := filepath.Join("..", "..", "tests", "fixtures", "stage_open_root.usda")
	st := OpenStageFromRootFile(root, RootSubDepthFirst)
	if st == nil {
		t.Fatal("OpenStageFromRootFile:", LastErrorMessage())
	}
	defer st.Free()
	if !st.PrimPathInUse("/FromSub") {
		t.Fatal("expected composed prim /FromSub from sublayer")
	}
	st0 := OpenStageFromRootFile(root, RootSubNone)
	if st0 == nil {
		t.Fatal("open none:", LastErrorMessage())
	}
	defer st0.Free()
	if st0.PrimPathInUse("/FromSub") {
		t.Fatal("expected no /FromSub when sublayers disabled")
	}
}

func TestComposedRelocates(t *testing.T) {
	const usda = `#usda 1.0
(
    relocates = {
        </Src>: </Dst>,
    }
)
def Xform "Src"
(
)
{
}
def Xform "Dst"
(
)
{
}
`
	l := NewAnonymousLayer("go_reloc")
	if l == nil {
		t.Fatal("layer:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA %d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("stage:", LastErrorMessage())
	}
	defer st.Free()
	if st.RelocateSourceInAnyLayer("/Src") != 1 {
		t.Fatal("expected relocate for /Src")
	}
	tgt, rc := st.ComposedRelocateTarget("/Src")
	if rc != 0 || tgt != "/Dst" {
		t.Fatalf("target rc=%d tgt=%q %s", rc, tgt, LastErrorMessage())
	}
	pairs, rc := st.ListComposedRelocatePairs()
	if rc != 0 || len(pairs) != 1 {
		t.Fatalf("pairs rc=%d n=%d %s", rc, len(pairs), LastErrorMessage())
	}
	parts := strings.SplitN(pairs[0], RelocatePairSep, 2)
	if len(parts) != 2 || parts[0] != "/Src" || parts[1] != "/Dst" {
		t.Fatalf("pair %q", pairs[0])
	}
	if st.RelocateSourceInAnyLayer("/Nope") != 0 {
		t.Fatal("expected 0 for /Nope")
	}
}

func TestComposedPrefixSubstitutions(t *testing.T) {
	const usda = `#usda 1.0
(
    prefixSubstitutions = {
        "/Models": "/ModelsV2",
    }
)
def Xform "Root"
(
)
{
}
`
	l := NewAnonymousLayer("go_psub")
	if l == nil {
		t.Fatal("layer:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA %d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("stage:", LastErrorMessage())
	}
	defer st.Free()
	if st.PrefixSubstitutionKeyInAnyLayer("/Models") != 1 {
		t.Fatal("expected key /Models")
	}
	tgt, rc := st.ComposedPrefixSubstitution("/Models")
	if rc != 0 || tgt != "/ModelsV2" {
		t.Fatalf("target rc=%d tgt=%q %s", rc, tgt, LastErrorMessage())
	}
	pairs, rc := st.ListComposedPrefixSubstitutionPairs()
	if rc != 0 || len(pairs) != 1 {
		t.Fatalf("pairs rc=%d n=%d %s", rc, len(pairs), LastErrorMessage())
	}
	parts := strings.SplitN(pairs[0], RelocatePairSep, 2)
	if len(parts) != 2 || parts[0] != "/Models" || parts[1] != "/ModelsV2" {
		t.Fatalf("pair %q", pairs[0])
	}
	if st.PrefixSubstitutionKeyInAnyLayer("/Nope") != 0 {
		t.Fatal("expected 0 for /Nope")
	}
}

func TestComposedCustomLayerData(t *testing.T) {
	const usda = `#usda 1.0
(
    customLayerData = {
        string layerTag = "hello",
        string layerBuild = "2025",
    }
)
def "R"
(
)
{
}
`
	l := NewAnonymousLayer("go_cld")
	if l == nil {
		t.Fatal("layer:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA %d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("stage:", LastErrorMessage())
	}
	defer st.Free()
	if st.CustomLayerDataKeyInAnyLayer("layerTag") != 1 {
		t.Fatal("expected layerTag")
	}
	v, rc := st.ComposedCustomLayerData("layerTag")
	if rc != 0 || v != "hello" {
		t.Fatalf("value rc=%d v=%q %s", rc, v, LastErrorMessage())
	}
	keys, rc := st.ListComposedCustomLayerDataKeys()
	if rc != 0 || len(keys) != 2 {
		t.Fatalf("keys rc=%d n=%d %s", rc, len(keys), LastErrorMessage())
	}
	if keys[0] != "layerBuild" || keys[1] != "layerTag" {
		t.Fatalf("keys %v", keys)
	}
	if st.CustomLayerDataKeyInAnyLayer("nope") != 0 {
		t.Fatal("expected 0 for nope")
	}
}

func TestComposedPrimVariant(t *testing.T) {
	const usda = `#usda 1.0
(
)
def Xform "Root"
(
    variantSelection = {
        string modelVariant = "HiPoly",
    }
    variantSets = {
        modelVariant = {
            "HiPoly" = {},
            LoPoly = {},
        }
    }
)
{
}
`
	l := NewAnonymousLayer("go_var")
	if l == nil {
		t.Fatal("layer:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA %d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("stage:", LastErrorMessage())
	}
	defer st.Free()
	if st.PrimVariantSelectionSetInAnyLayer("/Root", "modelVariant") != 1 {
		t.Fatal("expected variantSelection")
	}
	sel, rc := st.ComposedPrimVariantSelection("/Root", "modelVariant")
	if rc != 0 || sel != "HiPoly" {
		t.Fatalf("selection rc=%d sel=%q %s", rc, sel, LastErrorMessage())
	}
	sets, rc := st.ListComposedPrimVariantSelectionSets("/Root")
	if rc != 0 || len(sets) != 1 || sets[0] != "modelVariant" {
		t.Fatalf("selection sets %v rc=%d", sets, rc)
	}
	if st.PrimVariantSetDeclaredInAnyLayer("/Root", "modelVariant") != 1 {
		t.Fatal("expected variantSet")
	}
	vsets, rc := st.ListComposedPrimVariantSetNames("/Root")
	if rc != 0 || len(vsets) != 1 || vsets[0] != "modelVariant" {
		t.Fatalf("variant set names %v rc=%d", vsets, rc)
	}
	names, rc := st.ListComposedPrimVariantNames("/Root", "modelVariant")
	if rc != 0 || len(names) != 2 || names[0] != "HiPoly" || names[1] != "LoPoly" {
		t.Fatalf("variant names %v rc=%d", names, rc)
	}
	_, rc = st.ComposedPrimVariantSelection("/Root", "missing")
	if rc != 3 {
		t.Fatalf("expected NOT_FOUND 3 got %d", rc)
	}
	_, rc = st.ListComposedPrimVariantNames("/Root", "missing")
	if rc != 3 {
		t.Fatalf("expected NOT_FOUND 3 for names got %d", rc)
	}
}

func TestResolvePrimSpecifierKind(t *testing.T) {
	const usda = `#usda 1.0
(
)
class Xform "P"
(
)
{
}
over Xform "O"
(
)
{
}
def Xform "Q"
(
)
{
}
`
	l := NewAnonymousLayer("go_spec")
	if l == nil {
		t.Fatal("layer:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA %d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("stage:", LastErrorMessage())
	}
	defer st.Free()
	if st.ResolvePrimSpecifierKind("/P") != PrimSpecifierClass {
		t.Fatal("/P")
	}
	if st.ResolvePrimSpecifierKind("/O") != PrimSpecifierOver {
		t.Fatal("/O")
	}
	if st.ResolvePrimSpecifierKind("/Q") != PrimSpecifierDefault {
		t.Fatal("/Q")
	}
	if st.ResolvePrimSpecifierKind("not_a_path") >= 0 {
		t.Fatal("expected error for bad path")
	}
}

func TestLayerHints(t *testing.T) {
	const usda = `#usda 1.0
(
    startTimeCode = 0
    endTimeCode = 100
    timeCodesPerSecond = 30
    framesPerSecond = 30
    framePrecision = 2
    metersPerUnit = 0.01
    upAxis = "Z"
    primOrder = [</Root/A>, </Root>]
)
def Xform "Root"
{
    def "A"
    {
    }
}
`
	l := NewAnonymousLayer("go_hints")
	if l == nil {
		t.Fatal("layer:", LastErrorMessage())
	}
	defer l.Free()
	if rc := l.LoadUSDA(usda); rc != 0 {
		t.Fatalf("LoadUSDA %d %s", rc, LastErrorMessage())
	}
	st := AttachRootLayer(l)
	if st == nil {
		t.Fatal("stage:", LastErrorMessage())
	}
	defer st.Free()
	v, has, rc := st.StartTimeCode()
	if rc != 0 || !has || v != 0 {
		t.Fatalf("start %v %v rc=%d", v, has, rc)
	}
	v, has, rc = st.EndTimeCode()
	if rc != 0 || !has || v != 100 {
		t.Fatalf("end %v", v)
	}
	v, has, rc = st.TimeCodesPerSecond()
	if rc != 0 || !has || v != 30 {
		t.Fatalf("tcps %v", v)
	}
	v, has, rc = st.FramesPerSecond()
	if rc != 0 || !has || v != 30 {
		t.Fatalf("fps %v", v)
	}
	iv, ihas, rc := st.FramePrecision()
	if rc != 0 || !ihas || iv != 2 {
		t.Fatalf("framePrecision %v", iv)
	}
	v, has, rc = st.MetersPerUnit()
	if rc != 0 || !has || v != 0.01 {
		t.Fatalf("mpu %v", v)
	}
	axis, rc := st.UpAxis()
	if rc != 0 || axis != "Z" {
		t.Fatalf("upAxis %q rc=%d", axis, rc)
	}
	paths, rc := st.PrimOrderPaths()
	if rc != 0 || len(paths) != 2 || paths[0] != "/Root/A" || paths[1] != "/Root" {
		t.Fatalf("primOrder %v rc=%d", paths, rc)
	}
}
