import { ChangeDetectionStrategy, Component, computed, inject, input } from '@angular/core';
import { MarkdownService } from '../core/markdown.service';
import { CopyCodeDirective } from './copy-code.directive';

@Component({
  selector: 'docs-markdown',
  imports: [CopyCodeDirective],
  template: `<div class="prose-doc" [innerHTML]="rendered().html" docsCopyCode></div>`,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class MarkdownView {
  readonly markdown = input.required<string>();

  private readonly markdownService = inject(MarkdownService);
  protected readonly rendered = computed(() => this.markdownService.render(this.markdown()));
}
