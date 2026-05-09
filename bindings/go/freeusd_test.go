package freeusd

import "testing"

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
