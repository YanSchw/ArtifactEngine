import { ChangeDetectionStrategy, Component, computed, effect, inject, input } from '@angular/core';
import { Title } from '@angular/platform-browser';
import { RouterLink } from '@angular/router';
import { ApiDocsService } from '../../core/api-docs.service';
import { KindBadge } from '../../shared/kind-badge';
import { TypeKind } from '../../core/api.models';

@Component({
  selector: 'docs-module-page',
  imports: [RouterLink, KindBadge],
  templateUrl: './module-page.html',
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class ModulePage {
  readonly name = input.required<string>();

  protected readonly docs = inject(ApiDocsService);
  private readonly title = inject(Title);

  protected readonly module = computed(() => this.docs.getModule(this.name()));
  protected readonly importedBy = computed(() =>
    this.docs.modules().filter((m) => m.importModules.includes(this.name())),
  );

  protected readonly typeGroups = computed(() => {
    const types = this.docs.getTypesOfModule(this.name());
    return (['class', 'struct', 'enum'] as TypeKind[])
      .map((kind) => ({ kind, types: types.filter((t) => t.kind === kind) }))
      .filter((group) => group.types.length > 0);
  });

  constructor() {
    effect(() => this.title.setTitle(`${this.name()} Module — Artifact Engine`));
  }
}
