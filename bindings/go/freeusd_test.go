package freeusd

import (
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
