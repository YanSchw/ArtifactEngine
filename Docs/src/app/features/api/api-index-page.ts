import { ChangeDetectionStrategy, Component, computed, inject, signal } from '@angular/core';
import { RouterLink } from '@angular/router';
import { ApiDocsService } from '../../core/api-docs.service';
import { KindBadge } from '../../shared/kind-badge';
import { TypeKind } from '../../core/api.models';

@Component({
  selector: 'docs-api-index-page',
  imports: [RouterLink, KindBadge],
  templateUrl: './api-index-page.html',
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class ApiIndexPage {
  protected readonly docs = inject(ApiDocsService);

  protected readonly query = signal('');
  protected readonly kindFilter = signal<TypeKind | null>(null);
  protected readonly kinds: TypeKind[] = ['class', 'struct', 'enum'];

  protected readonly filtered = computed(() => {
    const query = this.query().trim().toLowerCase();
    const kind = this.kindFilter();
    return this.docs
      .types()
      .filter((t) => !kind || t.kind === kind)
      .filter(
        (t) =>
          !query ||
          t.name.toLowerCase().includes(query) ||
          t.module.toLowerCase().includes(query),
      );
  });

  protected toggleKind(kind: TypeKind): void {
    this.kindFilter.update((current) => (current === kind ? null : kind));
  }
}
