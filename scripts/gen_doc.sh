#!/bin/sh
# scripts/gen_doc.sh — genere la reference des directives depuis la table
# unique `src/schema/Directives.cpp` (T021, regle R1) et l'injecte dans
# `docs/FORMAT_SCENE.md` entre les marqueurs SCHEMA-GENERATED-START/END.
#
# Aucune definition dupliquee : le Markdown est produit par
# `rt::schema::writeMarkdown()`, qui lit la meme table que le parser
# (find), la validation (check*) et la future UI (all). Ajouter une
# directive = une seule ligne dans la table, puis relancer ce script.
#
# Usage : sh scripts/gen_doc.sh
# Idempotent : une seconde execution ne change rien.
set -u

cd "$(dirname "$0")/.." || exit 1

DOC="docs/FORMAT_SCENE.md"
START="<!-- SCHEMA-GENERATED-START -->"
END="<!-- SCHEMA-GENERATED-END -->"

if ! grep -q "$START" "$DOC" || ! grep -q "$END" "$DOC"; then
	echo "gen_doc: marqueurs absents dans $DOC (attendu $START / $END)" >&2
	exit 1
fi

TMPDIR="$(mktemp -d 2>/dev/null || echo /tmp/rt-gen-doc-$$)"
mkdir -p "$TMPDIR" || exit 1
DUMP_CPP="$TMPDIR/dump.cpp"
DUMP_BIN="$TMPDIR/dump"
DUMP_MD="$TMPDIR/schema.md"

cleanup() {
	rm -rf "$TMPDIR"
}
trap cleanup INT TERM

cat > "$DUMP_CPP" <<'EOF'
#include <iostream>
#include "rt/schema/Directives.hpp"
int main() {
    rt::schema::writeMarkdown(std::cout);
    return 0;
}
EOF

if ! c++ -Wall -Wextra -Werror -O2 -std=c++2c -Iinclude src/schema/Directives.cpp "$DUMP_CPP" -o "$DUMP_BIN"; then
	echo "gen_doc: compilation du dumper impossible" >&2
	rm -rf "$TMPDIR"
	exit 1
fi

if ! "$DUMP_BIN" > "$DUMP_MD"; then
	echo "gen_doc: execution du dumper impossible" >&2
	rm -rf "$TMPDIR"
	exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
	echo "gen_doc: python3 introuvable (requis pour la splicing)" >&2
	rm -rf "$TMPDIR"
	exit 1
fi

PYTHON_OUT="$TMPDIR/splice_rc"
DOC="$DOC" START="$START" END="$END" DUMP_MD="$DUMP_MD" python3 - <<'PYEOF'
import os
doc_path = os.environ["DOC"]
start = os.environ["START"]
end = os.environ["END"]
dump_path = os.environ["DUMP_MD"]
with open(doc_path, "r", encoding="utf-8") as f:
    content = f.read()
with open(dump_path, "r", encoding="utf-8") as f:
    generated = f.read().rstrip("\n")
start_idx = content.find(start)
end_idx = content.find(end)
if start_idx < 0 or end_idx < 0 or end_idx < start_idx:
    raise SystemExit("marqueurs introuvables ou inverses")
before = content[: start_idx + len(start)]
after = content[end_idx:]
new_content = before + "\n\n" + generated + "\n\n" + after
with open(doc_path, "w", encoding="utf-8") as f:
    f.write(new_content)
PYEOF
# shellcheck disable=SC2181
if [ "$?" -ne 0 ]; then
	echo "gen_doc: splicing impossible" >&2
	rm -rf "$TMPDIR"
	exit 1
fi

rm -rf "$TMPDIR"
trap - INT TERM
echo "gen_doc: $DOC mis a jour depuis src/schema/Directives.cpp"
