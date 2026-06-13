package freeusd

import (
	"encoding/binary"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func readFixtureUSDA(t *testing.T, elems ...string) string {
	t.Helper()
	path := filepath.Join(elems...)
	data, err := os.ReadFile(path)
	if err != nil {
		t.Fatalf("ReadFile(%s): %v", path, err)
	}
	return string(data)
}

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

func TestReadUsdcSectionBytesFromPath(t *testing.T) {
	dir := t.TempDir()
	p := filepath.Join(dir, "section.usdc")
	buf := make([]byte, 160)
	copy(buf[0:], UsdcCrateIdentifier())
	buf[8] = 0
	buf[9] = 8
	buf[10] = 0
	binary.LittleEndian.PutUint64(buf[16:24], uint64(88))
	binary.LittleEndian.PutUint64(buf[88:96], 1)
	copy(buf[96:103], []byte("TOKENS\x00"))
	binary.LittleEndian.PutUint64(buf[112:120], uint64(128))
	binary.LittleEndian.PutUint64(buf[120:128], uint64(5))
	copy(buf[128:133], []byte("alpha"))
	if err := os.WriteFile(p, buf, 0o644); err != nil {
		t.Fatal(err)
	}
	payload, rc := ReadUsdcSectionBytesFromPath(p, "TOKENS", 64)
	if rc != 0 {
		t.Fatalf("section rc=%d %s", rc, LastErrorMessage())
	}
	if string(payload) != "alpha" {
		t.Fatalf("unexpected payload %q", string(payload))
	}
	_, rcMissing := ReadUsdcSectionBytesFromPath(p, "MISSING", 64)
	if rcMissing == 0 {
		t.Fatal("expected missing section error")
	}
}

func TestReadStructuredUsdcTablesFromFixture(t *testing.T) {
	p := filepath.Join("..", "..", "tests", "fixtures", "parity_tables.usdc")
	tokens, rc := ReadUsdcTokenTableFromPath(p, 8, 1024)
	if rc != 0 {
		t.Fatalf("token table rc=%d %s", rc, LastErrorMessage())
	}
	if len(tokens) != 2 || tokens[0] != "render" || tokens[1] != "invisible" {
		t.Fatalf("unexpected tokens %#v", tokens)
	}
	strings, rc := ReadUsdcStringTableFromPath(p, 8, 1024)
	if rc != 0 {
		t.Fatalf("string table rc=%d %s", rc, LastErrorMessage())
	}
	if len(strings) != 2 || strings[0] != "hello" || strings[1] != "world" {
		t.Fatalf("unexpected strings %#v", strings)
	}
	paths, rc := ReadUsdcPathTableFromPath(p, 8, 1024)
	if rc != 0 {
		t.Fatalf("path table rc=%d %s", rc, LastErrorMessage())
	}
	if len(paths) != 2 || paths[0] != "/World" || paths[1] != "/World/Cube" {
		t.Fatalf("unexpected paths %#v", paths)
	}
	fields, rc := ReadUsdcFieldsTableFromPath(p, 8, 1024)
	if rc != 0 {
		t.Fatalf("fields table rc=%d %s", rc, LastErrorMessage())
	}
	if len(fields) != 2 || fields[0].TokenIndex != 0 || fields[0].ValueTypeTokenIndex != 1 ||
		fields[1].TokenIndex != 1 || fields[1].ValueTypeTokenIndex != 0 {
		t.Fatalf("unexpected fields %#v", fields)
	}
	specs, rc := ReadUsdcSpecsTableFromPath(p, 8, 1024)
	if rc != 0 {
		t.Fatalf("specs table rc=%d %s", rc, LastErrorMessage())
	}
	if len(specs) != 2 || specs[0].PathIndex != 0 || specs[0].FieldSetIndex != 0 || specs[0].SpecType != 1 ||
		specs[1].PathIndex != 1 || specs[1].FieldSetIndex != 1 || specs[1].SpecType != 2 {
		t.Fatalf("unexpected specs %#v", specs)
	}
	sets, rc := ReadUsdcFieldSetsTableFromPath(p, 8, 8, 1024)
	if rc != 0 {
		t.Fatalf("fieldsets table rc=%d %s", rc, LastErrorMessage())
	}
	if len(sets) != 2 || len(sets[0].FieldIndices) != 2 || sets[0].FieldIndices[0] != 0 || sets[0].FieldIndices[1] != 1 ||
		len(sets[1].FieldIndices) != 1 || sets[1].FieldIndices[0] != 1 {
		t.Fatalf("unexpected fieldsets %#v", sets)
	}

	values, rc := ReadUsdcValuesTableFromPath(p, 16, 1024)
	if rc != 0 {
		t.Fatalf("values table rc=%d %s", rc, LastErrorMessage())
	}
	if len(values) != 15 || len(values[0].Bytes) != 4 {
		t.Fatalf("unexpected values %#v", values)
	}
	typed, rc := ReadUsdcTypedValuesTableFromPath(p, 16, 1024)
	if rc != 0 {
		t.Fatalf("typed values table rc=%d %s", rc, LastErrorMessage())
	}
	if len(typed) != 15 || typed[0].Kind != 1 || typed[0].Int32Value != 42 || typed[1].Kind != 2 ||
		typed[2].Kind != 3 || typed[2].TokenIndex != 0 || typed[3].Kind != 4 || !typed[3].BoolValue ||
		typed[14].Kind != 15 || typed[14].Vec4fValue[3] < 3.99 {
		t.Fatalf("unexpected typed values %#v", typed)
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

func TestCrossLanguageFieldReadContract(t *testing.T) {
	usda := readFixtureUSDA(t, "..", "..", "tests", "fixtures", "usd_cross_language.usda")
	l := NewAnonymousLayer("go_cross_lang")
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

	if ok := st.PrimPathInUse("/Scene/Child"); !ok {
		t.Fatalf("PrimPathInUse child ok=%v %s", ok, LastErrorMessage())
	}
	if ok := st.PrimPathInUse("/Scene/Missing"); ok {
		t.Fatalf("PrimPathInUse missing ok=%v %s", ok, LastErrorMessage())
	}

	if v, rc := st.ReadFieldDouble("/Scene/Child", "mass", 1.0); rc != 0 || v != 2.5 {
		t.Fatalf("ReadFieldDouble mass rc=%d v=%v %s", rc, v, LastErrorMessage())
	}
	if v, rc := st.ReadFieldDouble("/Scene/Child", "mass", 2.0); rc != 0 || v != 4.0 {
		t.Fatalf("ReadFieldDouble mass@2 rc=%d v=%v %s", rc, v, LastErrorMessage())
	}
	if f, rc := st.ReadFieldFloat("/Scene/Child", "density", 1.0); rc != 0 || f != 1.25 {
		t.Fatalf("ReadFieldFloat density rc=%d f=%v %s", rc, f, LastErrorMessage())
	}
	if f, rc := st.ReadFieldFloat("/Scene/Child", "mass", 1.0); rc != 0 || f != 2.5 {
		t.Fatalf("ReadFieldFloat mass rc=%d f=%v %s", rc, f, LastErrorMessage())
	}
	if b, rc := st.ReadFieldBool("/Scene/Child", "enabled", 1.0); rc != 0 || !b {
		t.Fatalf("ReadFieldBool enabled rc=%d b=%v %s", rc, b, LastErrorMessage())
	}
	if n, rc := st.ReadFieldInt64("/Scene/Child", "count", 1.0); rc != 0 || n != -7 {
		t.Fatalf("ReadFieldInt64 count rc=%d n=%d %s", rc, n, LastErrorMessage())
	}
	if n, rc := st.ReadFieldInt64("/Scene/Child", "mass", 1.0); rc != 0 || n != 2 {
		t.Fatalf("ReadFieldInt64 mass rc=%d n=%d %s", rc, n, LastErrorMessage())
	}
	if s, rc := st.ReadFieldString("/Scene/Child", "label", 1.0); rc != 0 || s != "hello" {
		t.Fatalf("ReadFieldString label rc=%d s=%q %s", rc, s, LastErrorMessage())
	}
	if s, rc := st.ReadFieldString("/Scene/Child", "kind", 1.0); rc != 0 || s != "component" {
		t.Fatalf("ReadFieldString kind rc=%d s=%q %s", rc, s, LastErrorMessage())
	}
	if x, y, z, rc := st.ReadFieldVec3d("/Scene/Child", "extent", 1.0); rc != 0 || x != 1 || y != 2 || z != 3 {
		t.Fatalf("ReadFieldVec3d extent rc=%d (%v,%v,%v) %s", rc, x, y, z, LastErrorMessage())
	}
	if x, y, z, rc := st.ReadFieldVec3f("/Scene/Child", "displayColor", 1.0); rc != 0 || x != 0.25 || y != 0.5 || z != 0.75 {
		t.Fatalf("ReadFieldVec3f color rc=%d (%v,%v,%v) %s", rc, x, y, z, LastErrorMessage())
	}
	if x, y, z, rc := st.ReadFieldVec3f("/Scene/Child", "extent", 1.0); rc != 0 || x != 1 || y != 2 || z != 3 {
		t.Fatalf("ReadFieldVec3f extent rc=%d (%v,%v,%v) %s", rc, x, y, z, LastErrorMessage())
	}
	if m, rc := st.ReadFieldMatrix4d("/Scene/Child", "xf", 1.0); rc != 0 || m[0] != 1 || m[5] != 1 || m[10] != 1 || m[15] != 1 {
		t.Fatalf("ReadFieldMatrix4d xf rc=%d m=%v %s", rc, m, LastErrorMessage())
	}
	if real, i, j, k, rc := st.ReadFieldQuatd("/Scene/Child", "qd", 1.0); rc != 0 || real != 1 || i != 0 || j != 0 || k != 0 {
		t.Fatalf("ReadFieldQuatd qd rc=%d (%v,%v,%v,%v) %s", rc, real, i, j, k, LastErrorMessage())
	}
	if real, i, j, k, rc := st.ReadFieldQuatf("/Scene/Child", "qf", 1.0); rc != 0 || real != 0.70710677 || i != 0 || j != 0 || k != 0.70710677 {
		t.Fatalf("ReadFieldQuatf qf rc=%d (%v,%v,%v,%v) %s", rc, real, i, j, k, LastErrorMessage())
	}
	if real, i, j, k, rc := st.ReadFieldQuatf("/Scene/Child", "qd", 1.0); rc != 0 || real != 1 || i != 0 || j != 0 || k != 0 {
		t.Fatalf("ReadFieldQuatf qd rc=%d (%v,%v,%v,%v) %s", rc, real, i, j, k, LastErrorMessage())
	}
	if tok, rc := st.ReadFieldToken("/Scene/Child", "kind", 1.0); rc != 0 || tok != "component" {
		t.Fatalf("ReadFieldToken kind rc=%d tok=%q %s", rc, tok, LastErrorMessage())
	}
	if tags, rc := st.ReadFieldTokenArray("/Scene/Child", "tags", 1.0); rc != 0 || len(tags) != 2 || tags[0] != "a" || tags[1] != "b" {
		t.Fatalf("ReadFieldTokenArray tags rc=%d tags=%v %s", rc, tags, LastErrorMessage())
	}

	if v, rc := st.ReadFieldDouble("/Scene/Child", "missingAttr", 1.0); rc == 0 || v != 0 {
		t.Fatalf("missing attr double rc=%d v=%v", rc, v)
	}
	if v, rc := st.ReadFieldDouble("/Scene/Missing", "mass", 1.0); rc == 0 || v != 0 {
		t.Fatalf("missing prim double rc=%d v=%v", rc, v)
	}
	if real, i, j, k, rc := st.ReadFieldQuatd("/Scene/Child", "qf", 1.0); rc == 0 || real != 0 || i != 0 || j != 0 || k != 0 {
		t.Fatalf("wrong-type quatd rc=%d (%v,%v,%v,%v)", rc, real, i, j, k)
	}
	if tok, rc := st.ReadFieldToken("/Scene/Child", "label", 1.0); rc == 0 || tok != "" {
		t.Fatalf("wrong-type token rc=%d tok=%q", rc, tok)
	}
	if tags, rc := st.ReadFieldTokenArray("/Scene/Child", "kind", 1.0); rc == 0 || tags != nil {
		t.Fatalf("wrong-type token array rc=%d tags=%v", rc, tags)
	}

	times, rc := st.ListFieldSampleTimes("/Scene/Child", "mass")
	if rc != 0 || len(times) != 1 || times[0] != 2.0 {
		t.Fatalf("ListFieldSampleTimes mass rc=%d times=%v %s", rc, times, LastErrorMessage())
	}
	if st.HasFieldOpinion("/Scene/Child", "mass") != 1 {
		t.Fatalf("HasFieldOpinion mass %s", LastErrorMessage())
	}
	if st.HasFieldOpinion("/Scene/Child", "missingAttr") != 0 {
		t.Fatalf("HasFieldOpinion missingAttr %s", LastErrorMessage())
	}
	tag, rc := st.ComposedPrimCustomDataInt64("/Scene/Child", "tag")
	if rc != 0 || tag != 99 {
		t.Fatalf("ComposedPrimCustomDataInt64 tag rc=%d tag=%d %s", rc, tag, LastErrorMessage())
	}
	if st.PrimCustomDataKeyInAnyLayer("/Scene/Child", "tag") != 1 {
		t.Fatalf("PrimCustomDataKeyInAnyLayer tag %s", LastErrorMessage())
	}
	keys, rc := st.ListComposedPrimCustomDataKeys("/Scene/Child")
	if rc != 0 || len(keys) != 1 || keys[0] != "tag" {
		t.Fatalf("ListComposedPrimCustomDataKeys rc=%d keys=%v %s", rc, keys, LastErrorMessage())
	}
	if st.HasPrimInherits("/Scene/ArcHost") != 1 {
		t.Fatalf("HasPrimInherits ArcHost %s", LastErrorMessage())
	}
	inh, rc := st.ListPrimInherits("/Scene/ArcHost")
	if rc != 0 || len(inh) != 1 || inh[0] != "/Scene/Child" {
		t.Fatalf("ListPrimInherits ArcHost rc=%d inh=%v %s", rc, inh, LastErrorMessage())
	}
	if st.HasPrimInherits("/Scene/Child") != 0 {
		t.Fatalf("HasPrimInherits Child %s", LastErrorMessage())
	}
}

// Regression: on C API failure, Go wrappers must not return uninitialized C out-parameters.
func TestStageReadFieldErrorsReturnZeroValues(t *testing.T) {
	const usda = `#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        double x = 1.0
    }
}
`
	l := NewAnonymousLayer("go_read_err")
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

	v, rc := st.ReadFieldDouble("/W/C", "no_such_attr", 1.0)
	if rc == 0 || v != 0 {
		t.Fatalf("ReadFieldDouble missing: rc=%d v=%v want rc!=0 v=0", rc, v)
	}
	f, rc := st.ReadFieldFloat("/W/C", "no_such_attr", 1.0)
	if rc == 0 || f != 0 {
		t.Fatalf("ReadFieldFloat missing: rc=%d f=%v", rc, f)
	}
	x, y, z, rc := st.ReadFieldVec3d("/W/C", "no_such_attr", 1.0)
	if rc == 0 || x != 0 || y != 0 || z != 0 {
		t.Fatalf("ReadFieldVec3d missing: rc=%d (%v,%v,%v)", rc, x, y, z)
	}
	fx, fy, fz, rc := st.ReadFieldVec3f("/W/C", "no_such_attr", 1.0)
	if rc == 0 || fx != 0 || fy != 0 || fz != 0 {
		t.Fatalf("ReadFieldVec3f missing: rc=%d", rc)
	}
	qr, qi, qj, qk, rc := st.ReadFieldQuatd("/W/C", "no_such_attr", 1.0)
	if rc == 0 || qr != 0 || qi != 0 || qj != 0 || qk != 0 {
		t.Fatalf("ReadFieldQuatd missing: rc=%d", rc)
	}
	fr, fi, fj, fk, rc := st.ReadFieldQuatf("/W/C", "no_such_attr", 1.0)
	if rc == 0 || fr != 0 || fi != 0 || fj != 0 || fk != 0 {
		t.Fatalf("ReadFieldQuatf missing: rc=%d", rc)
	}
	n, rc := st.ReadFieldInt64("/W/C", "no_such_attr", 1.0)
	if rc == 0 || n != 0 {
		t.Fatalf("ReadFieldInt64 missing: rc=%d n=%d", rc, n)
	}
	b, rc := st.ReadFieldBool("/W/C", "no_such_attr", 1.0)
	if rc == 0 || b {
		t.Fatalf("ReadFieldBool missing: rc=%d b=%v", rc, b)
	}
	s, rc := st.ReadFieldString("/W/C", "no_such_attr", 1.0)
	if rc == 0 || s != "" {
		t.Fatalf("ReadFieldString missing: rc=%d s=%q", rc, s)
	}
	m, rc := st.ReadFieldMatrix4d("/W/C", "no_such_attr", 1.0)
	if rc == 0 {
		t.Fatalf("ReadFieldMatrix4d missing: want rc!=0")
	}
	for i := 0; i < 16; i++ {
		if m[i] != 0 {
			t.Fatalf("ReadFieldMatrix4d missing: m[%d]=%v want 0", i, m[i])
		}
	}
	tok, rc := st.ReadFieldToken("/W/C", "no_such_attr", 1.0)
	if rc == 0 || tok != "" {
		t.Fatalf("ReadFieldToken missing: rc=%d tok=%q", rc, tok)
	}
	tags, rc := st.ReadFieldTokenArray("/W/C", "no_such_attr", 1.0)
	if rc == 0 || tags != nil {
		t.Fatalf("ReadFieldTokenArray missing: rc=%d tags=%#v want rc!=0 tags=nil", rc, tags)
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

func TestComposedPrimKindActiveFromArcs(t *testing.T) {
	fixture := filepath.Join("..", "..", "tests", "fixtures", "parity_kind_active_refs.usda")
	st := OpenStageFromRootFile(fixture, RootSubDepthFirst)
	if st == nil {
		t.Fatal("OpenStageFromRootFile:", LastErrorMessage())
	}
	defer st.Free()

	kind, rc := st.ResolvePrimKind("/World/RefHost")
	if rc != 0 || kind != "component" {
		t.Fatalf("ResolvePrimKind RefHost kind=%q rc=%d %s", kind, rc, LastErrorMessage())
	}
	if st.ResolveHasPrimKind("/World/RefHost") != 1 {
		t.Fatalf("ResolveHasPrimKind RefHost %s", LastErrorMessage())
	}
	active, rc := st.ResolvePrimActive("/World/RefHost")
	if rc != 0 || active {
		t.Fatalf("ResolvePrimActive RefHost active=%v rc=%d %s", active, rc, LastErrorMessage())
	}
	if st.ResolveHasPrimActiveOpinion("/World/RefHost") != 1 {
		t.Fatalf("ResolveHasPrimActiveOpinion RefHost %s", LastErrorMessage())
	}

	kind, rc = st.ResolvePrimKind("/World/PayloadHost")
	if rc != 0 || kind != "group" {
		t.Fatalf("ResolvePrimKind PayloadHost kind=%q rc=%d %s", kind, rc, LastErrorMessage())
	}
	active, rc = st.ResolvePrimActive("/World/InheritHost")
	if rc != 0 || !active {
		t.Fatalf("ResolvePrimActive InheritHost active=%v rc=%d %s", active, rc, LastErrorMessage())
	}
	if st.ResolveHasPrimActiveOpinion("/World/InheritHost") != 0 {
		t.Fatalf("ResolveHasPrimActiveOpinion InheritHost %s", LastErrorMessage())
	}
}

func TestUsdShadePreviewSurfaceBindings(t *testing.T) {
	fixture := filepath.Join("..", "..", "tests", "fixtures", "parity_shade_preview.usda")
	st := OpenStageFromRootFile(fixture, RootSubDepthFirst)
	if st == nil {
		t.Fatal("OpenStageFromRootFile preview:", LastErrorMessage())
	}
	defer st.Free()

	shaderPath, rc := st.ReadMaterialSurfaceShaderPath("/World/Looks/Material")
	if rc != 0 || shaderPath != "/World/Looks/Material/PreviewSurface" {
		t.Fatalf("ReadMaterialSurfaceShaderPath path=%q rc=%d %s", shaderPath, rc, LastErrorMessage())
	}
	rgb, rc := st.ReadPreviewSurfaceDiffuseColor(shaderPath, 1.0)
	if rc != 0 || rgb[0] != 0.8 || rgb[1] != 0.2 || rgb[2] != 0.1 {
		t.Fatalf("ReadPreviewSurfaceDiffuseColor rgb=%v rc=%d %s", rgb, rc, LastErrorMessage())
	}

	textureFixture := filepath.Join("..", "..", "tests", "fixtures", "parity_shade_texture.usda")
	textureStage := OpenStageFromRootFile(textureFixture, RootSubDepthFirst)
	if textureStage == nil {
		t.Fatal("OpenStageFromRootFile texture:", LastErrorMessage())
	}
	defer textureStage.Free()
	shaderPath, rc = textureStage.ReadMaterialSurfaceShaderPath("/World/Looks/Material")
	if rc != 0 {
		t.Fatalf("texture shader path rc=%d %s", rc, LastErrorMessage())
	}
	texturePath, rc := textureStage.ReadPreviewSurfaceDiffuseTextureAssetPath(shaderPath, 1.0)
	if rc != 0 || texturePath != "textures/albedo.png" {
		t.Fatalf("ReadPreviewSurfaceDiffuseTextureAssetPath path=%q rc=%d %s", texturePath, rc, LastErrorMessage())
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

func TestUsdGeomEngineSubsetParityImageable(t *testing.T) {
	fixture := filepath.Join("..", "..", "tests", "fixtures", "parity_imageable.usda")
	st := OpenStageFromRootFile(fixture, RootSubDepthFirst)
	if st == nil {
		t.Fatal("OpenStageFromRootFile:", LastErrorMessage())
	}
	defer st.Free()

	l2w, rc := st.ComputeLocalToWorldTransformMatrix4d("/World/Cube", 1.0)
	if rc != 0 {
		t.Fatalf("ComputeLocalToWorldTransformMatrix4d rc=%d %s", rc, LastErrorMessage())
	}
	if l2w[12] != 1.0 || l2w[13] != 2.0 || l2w[14] != 3.0 || l2w[15] != 1.0 {
		t.Fatalf("unexpected l2w translation %v", l2w)
	}

	vis, rc := st.ComputeImageableVisibility("/World/Cube", 1.0)
	if rc != 0 || vis {
		t.Fatalf("visibility visible=%v rc=%d", vis, rc)
	}

	purpose, rc := st.ComputeImageablePurpose("/World/Cube", 1.0)
	if rc != 0 || purpose != "render" {
		t.Fatalf("purpose %q rc=%d", purpose, rc)
	}

	minX, minY, minZ, maxX, maxY, maxZ, rc := st.ComputeBoundableWorldBounds("/World/Cube", 1.0)
	if rc != 0 {
		t.Fatalf("world bounds rc=%d %s", rc, LastErrorMessage())
	}
	if minX != 0 || minY != 1 || minZ != 2 || maxX != 2 || maxY != 3 || maxZ != 4 {
		t.Fatalf("world bounds (%g,%g,%g)-(%g,%g,%g)", minX, minY, minZ, maxX, maxY, maxZ)
	}

	_, _, _, _, _, _, rc = st.ComputeBoundableWorldBounds("/World", 1.0)
	if rc == 0 {
		t.Fatal("expected NOT_FOUND for non-boundable /World")
	}
}

func TestSkelCrossLanguageContract(t *testing.T) {
	fixture := filepath.Join("..", "..", "tests", "fixtures", "parity_skel_skinning.usda")
	st := OpenStageFromRootFile(fixture, RootSubDepthFirst)
	if st == nil {
		t.Fatalf("OpenStageFromRootFile: %s", LastErrorMessage())
	}
	defer st.Free()

	names, rc := st.ReadSkelJointNames("/World/SkelCharacter/Skeleton")
	if rc != 0 || len(names) != 2 || names[0] != "Root" || names[1] != "Root/Hip" {
		t.Fatalf("joint names %v rc=%d", names, rc)
	}

	report, rc := st.AssessEngineRuntimeSupport()
	if rc != 0 || !report.UsesSkelBoundMeshes || !report.UsesSkelAnimation {
		t.Fatalf("runtime report %+v rc=%d", report, rc)
	}

	points := [][3]float32{{0, 1, 0}}
	out, rc := st.DeformPointsWithSkeleton("/World/SkelCharacter/Skeleton", "/World/SkelCharacter/Anim", 1.0, points, []int32{1}, []float32{1}, 1)
	if rc != 0 || len(out) != 1 || out[0][1] <= points[0][1] {
		t.Fatalf("deform out=%v rc=%d", out, rc)
	}

	world := [][16]float64{}
	bind := [][16]float64{}
	for i := 0; i < 2; i++ {
		var w, b [16]float64
		w[0], w[5], w[10], w[15] = 1, 1, 1, 1
		b[0], b[5], b[10], b[15] = 1, 1, 1, 1
		if i == 1 {
			w[13] = 2
			b[13] = 1
		}
		world = append(world, w)
		bind = append(bind, b)
	}
	palette, rc := ComputeSkinningMatrices(world, bind)
	if rc != 0 || len(palette) != 2 || palette[1][13] != 3.0 {
		t.Fatalf("palette %v rc=%d", palette, rc)
	}
}
