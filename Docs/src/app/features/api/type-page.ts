import { ChangeDetectionStrategy, Component, computed, effect, inject, input } from '@angular/core';
import { httpResource } from '@angular/common/http';
import { Title } from '@angular/platform-browser';
import { RouterLink } from '@angular/router';
import { ApiDocsService } from '../../core/api-docs.service';
import { MarkdownService } from '../../core/markdown.service';
import { MarkdownView } from '../../shared/markdown-view';
import { KindBadge } from '../../shared/kind-badge';
import { ApiType } from '../../core/api.models';

@Component({
  selector: 'docs-type-page',
  imports: [RouterLink, KindBadge, MarkdownView],
  templateUrl: './type-page.html',
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class TypePage {
  readonly name = input.required<string>();

  protected readonly docs = inject(ApiDocsService);
  private readonly markdownService = inject(MarkdownService);
  private readonly title = inject(Title);

  protected readonly type = computed(() => this.docs.getType(this.name()));
  protected readonly inheritance = computed(() => {
    const type = this.type();
    return type ? this.docs.getInheritanceChain(type) : [];
  });
  protected readonly derived = computed(() => this.docs.getDerivedTypes(this.name()));

  /** Handwritten remarks placed at public/notes/<TypeName>.md; missing files simply render nothing. */
  private readonly notes = httpResource.text(() => `notes/${this.name()}.md`);
  protected readonly notesMarkdown = computed(() =>
    this.notes.error() === undefined ? this.notes.value() : undefined,
  );

  protected readonly syntax = computed(() => {
    const type = this.type();
    return type ? this.markdownService.highlightCpp(this.declarationOf(type)) : undefined;
  });

  /** True when the property type refers to another documented type (possibly wrapped in a container). */
  protected linkableType(propertyType: string): string | undefined {
    const inner = propertyType.replace(/^(Array|SharedObjectPtr)<(.+)>$/, '$2').trim();
    return this.docs.getType(inner) ? inner : undefined;
  }

  private declarationOf(type: ApiType): string {
    switch (type.kind) {
      case 'class':
        return type.parent
          ? `class ${type.name} : public ${type.parent} {\n    ARTIFACT_CLASS();\n};`
          : `class ${type.name} {\n};`;
      case 'struct':
        return `struct ${type.name} {\n    ARTIFACT_STRUCT();\n};`;
      case 'enum':
        return `ARTIFACT_ENUM();\nenum ${type.isEnumClass ? 'class ' : ''}${type.name} : ${type.underlyingType} {\n    /* ${type.values?.length ?? 0} values */\n};`;
    }
  }

  constructor() {
    effect(() => this.title.setTitle(`${this.name()} — Artifact Engine`));
  }
}
