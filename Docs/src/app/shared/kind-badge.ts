import { ChangeDetectionStrategy, Component, computed, input } from '@angular/core';
import { TypeKind } from '../core/api.models';

const KIND_STYLES: Record<TypeKind, string> = {
  class: 'bg-accent-500/15 text-accent-300 border-accent-500/40',
  struct: 'bg-teal-500/15 text-teal-400 border-teal-500/40',
  enum: 'bg-purple-500/15 text-purple-300 border-purple-500/40',
};

@Component({
  selector: 'docs-kind-badge',
  template: `
    <span
      class="inline-block rounded border px-1.5 py-0.5 font-mono text-[0.7rem] leading-none font-medium uppercase"
      [class]="style()"
    >{{ kind() }}</span>
  `,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class KindBadge {
  readonly kind = input.required<TypeKind>();
  protected readonly style = computed(() => KIND_STYLES[this.kind()]);
}
