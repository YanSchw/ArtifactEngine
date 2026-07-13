# Artifact Engine Documentation

Angular + TailwindCSS web app serving the engine documentation: handwritten markdown guides
and an API reference generated from the engine's reflection data.

## Development

```sh
# from the repo root, with the SDK venv active:
artifact docs         # dump reflection data into public/generated/api.json

cd Docs
npm install
npm start             # dev server at http://localhost:4200
npm run build         # production build into dist/
```

`public/generated/` is gitignored — run `artifact docs` after cloning or whenever reflected
types change.

## Content layout

| Path | Purpose |
| --- | --- |
| `public/generated/api.json` | Modules, classes, structs and enums dumped by `artifact docs`. |
| `public/guides/manifest.json` | Guide index (slug, title, summary) shown in nav and home page. |
| `public/guides/<slug>.md` | Handwritten guide pages. |
| `public/notes/<TypeName>.md` | Optional handwritten remarks rendered on a type's API page. |

To add a guide, drop a markdown file into `public/guides/` and register it in
`manifest.json`. To document a class by hand, create `public/notes/<ClassName>.md` — it
appears as the "Remarks" section of that type's page.
