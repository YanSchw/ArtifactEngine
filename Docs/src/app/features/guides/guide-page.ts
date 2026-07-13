import { ChangeDetectionStrategy, Component, computed, effect, inject, input } from '@angular/core';
import { httpResource } from '@angular/common/http';
import { Title } from '@angular/platform-browser';
import { RouterLink } from '@angular/router';
import { ApiDocsService } from '../../core/api-docs.service';
import { MarkdownService } from '../../core/markdown.service';
import { CopyCodeDirective } from '../../shared/copy-code.directive';

@Component({
  selector: 'docs-guide-page',
  imports: [RouterLink, CopyCodeDirective],
  templateUrl: './guide-page.html',
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class GuidePage {
  readonly slug = input.required<string>();

  protected readonly docs = inject(ApiDocsService);
  private readonly markdownService = inject(MarkdownService);
  private readonly title = inject(Title);

  private readonly source = httpResource.text(() => `guides/${this.slug()}.md`);

  protected readonly guide = computed(() => this.docs.guides().find((g) => g.slug === this.slug()));
  protected readonly loading = computed(() => this.source.isLoading());
  protected readonly missing = computed(() => this.source.error() !== undefined);
  protected readonly rendered = computed(() => {
    const markdown = this.source.value();
    return markdown !== undefined ? this.markdownService.render(markdown) : undefined;
  });

  constructor() {
    effect(() => {
      const name = this.guide()?.title ?? this.slug();
      this.title.setTitle(`${name} — Artifact Engine`);
    });
  }
}
