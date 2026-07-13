import { ChangeDetectionStrategy, Component, computed, inject, input } from '@angular/core';
import { MarkdownService } from '../core/markdown.service';

@Component({
  selector: 'docs-markdown',
  template: `<div class="prose-doc" [innerHTML]="rendered().html"></div>`,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class MarkdownView {
  readonly markdown = input.required<string>();

  private readonly markdownService = inject(MarkdownService);
  protected readonly rendered = computed(() => this.markdownService.render(this.markdown()));
}
