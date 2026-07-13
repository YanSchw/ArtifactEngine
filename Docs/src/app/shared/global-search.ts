import {
  ChangeDetectionStrategy,
  Component,
  ElementRef,
  HostListener,
  computed,
  effect,
  inject,
  signal,
  viewChild,
} from '@angular/core';
import { Router } from '@angular/router';
import { ApiDocsService } from '../core/api-docs.service';
import { TypeKind } from '../core/api.models';

interface SearchResult {
  section: 'Guides' | 'Types';
  kind: 'guide' | TypeKind;
  title: string;
  subtitle: string;
  link: (string | number)[];
  score: number;
}

const MAX_RESULTS = 20;

@Component({
  selector: 'docs-global-search',
  changeDetection: ChangeDetectionStrategy.OnPush,
  template: `
    @if (isOpen()) {
      <div
        class="fixed inset-0 z-50 flex items-start justify-center bg-black/50 px-4 pt-[12vh]"
        (click)="close()"
      >
        <div
          class="w-full max-w-xl overflow-hidden rounded-xl border border-line bg-ink-900 shadow-2xl"
          (click)="$event.stopPropagation()"
        >
          <div class="flex items-center gap-3 border-b border-line px-4">
            <svg class="h-4 w-4 shrink-0 text-mist-500" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round">
              <circle cx="11" cy="11" r="7" />
              <path d="m20 20-3.5-3.5" />
            </svg>
            <input
              #searchInput
              type="text"
              placeholder="Search guides and API types…"
              [value]="query()"
              (input)="onInput($any($event.target).value)"
              (keydown)="onKeydown($event)"
              class="w-full bg-transparent py-3.5 text-sm text-mist-100 placeholder-mist-500 focus:outline-none"
            />
            <kbd class="rounded border border-line bg-ink-800 px-1.5 py-0.5 font-mono text-[0.65rem] text-mist-500">Esc</kbd>
          </div>

          @if (results().length) {
            <ul #resultsList class="max-h-[60vh] overflow-y-auto py-2">
              @for (result of results(); track result.link; let i = $index) {
                <li>
                  <button
                    type="button"
                    (click)="go(result)"
                    (mouseenter)="activeIndex.set(i)"
                    class="flex w-full items-center gap-3 px-4 py-2 text-left"
                    [class]="i === activeIndex() ? 'bg-ink-700' : ''"
                  >
                    <span
                      class="inline-block w-14 shrink-0 rounded border px-1.5 py-0.5 text-center font-mono text-[0.6rem] leading-none font-medium uppercase"
                      [class]="badgeStyle(result.kind)"
                    >{{ result.kind === 'guide' ? 'Guide' : result.kind }}</span>
                    <span class="min-w-0 flex-1">
                      <span class="block truncate text-sm font-medium text-mist-100" [class.font-mono]="result.section === 'Types'">{{ result.title }}</span>
                      @if (result.subtitle) {
                        <span class="block truncate text-xs text-mist-500">{{ result.subtitle }}</span>
                      }
                    </span>
                  </button>
                </li>
              }
            </ul>
          } @else {
            <p class="px-4 py-8 text-center text-sm text-mist-500">
              No results for "{{ query() }}".
            </p>
          }
        </div>
      </div>
    }
  `,
})
export class GlobalSearch {
  private readonly docs = inject(ApiDocsService);
  private readonly router = inject(Router);
  private readonly searchInput = viewChild<ElementRef<HTMLInputElement>>('searchInput');
  private readonly resultsList = viewChild<ElementRef<HTMLUListElement>>('resultsList');

  protected readonly isOpen = signal(false);
  protected readonly query = signal('');
  protected readonly activeIndex = signal(0);

  private readonly index = computed<SearchResult[]>(() => {
    const entries: SearchResult[] = [];
    for (const guide of this.docs.guides()) {
      entries.push({
        section: 'Guides',
        kind: 'guide',
        title: guide.title,
        subtitle: guide.summary,
        link: ['/guides', guide.slug],
        score: 0,
      });
    }
    for (const type of this.docs.types()) {
      entries.push({
        section: 'Types',
        kind: type.kind,
        title: type.name,
        subtitle: type.module,
        link: ['/api/types', type.name],
        score: 0,
      });
    }
    return entries;
  });

  protected readonly results = computed<SearchResult[]>(() => {
    const q = this.query().trim().toLowerCase();
    if (!q) {
      return this.index()
        .filter((e) => e.section === 'Guides')
        .slice(0, MAX_RESULTS);
    }
    const scored: SearchResult[] = [];
    for (const entry of this.index()) {
      const score = this.score(entry, q);
      if (score > 0) scored.push({ ...entry, score });
    }
    scored.sort((a, b) => b.score - a.score || a.title.localeCompare(b.title));
    return scored.slice(0, MAX_RESULTS);
  });

  constructor() {
    effect(() => {
      this.results();
      this.activeIndex.set(0);
    });

    // searchInput() resolves only once the @if renders the field, so this reruns and focuses then.
    effect(() => {
      if (this.isOpen()) this.searchInput()?.nativeElement.focus();
    });

    // Keep the highlighted row visible during keyboard navigation.
    effect(() => {
      const list = this.resultsList()?.nativeElement;
      const item = list?.children.item(this.activeIndex());
      item?.scrollIntoView({ block: 'nearest' });
    });

    // Prevent the page behind the overlay from scrolling.
    effect(() => {
      document.body.style.overflow = this.isOpen() ? 'hidden' : '';
    });
  }

  @HostListener('document:keydown', ['$event'])
  onGlobalKeydown(event: KeyboardEvent): void {
    if ((event.metaKey || event.ctrlKey) && event.key.toLowerCase() === 'k') {
      event.preventDefault();
      this.isOpen() ? this.close() : this.open();
    } else if (event.key === 'Escape' && this.isOpen()) {
      event.preventDefault();
      this.close();
    }
  }

  open(): void {
    this.isOpen.set(true);
  }

  protected close(): void {
    this.isOpen.set(false);
    this.query.set('');
  }

  protected onInput(value: string): void {
    this.query.set(value);
  }

  protected onKeydown(event: KeyboardEvent): void {
    const count = this.results().length;
    switch (event.key) {
      case 'ArrowDown':
        event.preventDefault();
        if (count) this.activeIndex.set((this.activeIndex() + 1) % count);
        break;
      case 'ArrowUp':
        event.preventDefault();
        if (count) this.activeIndex.set((this.activeIndex() - 1 + count) % count);
        break;
      case 'Enter': {
        event.preventDefault();
        const result = this.results()[this.activeIndex()];
        if (result) this.go(result);
        break;
      }
    }
  }

  protected go(result: SearchResult): void {
    this.router.navigate(result.link);
    this.close();
  }

  protected badgeStyle(kind: SearchResult['kind']): string {
    switch (kind) {
      case 'class':
        return 'bg-accent-500/15 text-accent-300 border-accent-500/40';
      case 'struct':
        return 'bg-teal-500/15 text-teal-400 border-teal-500/40';
      case 'enum':
        return 'bg-purple-500/15 text-purple-300 border-purple-500/40';
      default:
        return 'bg-mist-500/15 text-mist-300 border-mist-500/40';
    }
  }

  private score(entry: SearchResult, q: string): number {
    const title = entry.title.toLowerCase();
    if (title === q) return 100;
    if (title.startsWith(q)) return 70;
    if (title.includes(q)) return 50;
    if (entry.subtitle.toLowerCase().includes(q)) return 20;
    return 0;
  }
}
