import { ChangeDetectionStrategy, Component } from '@angular/core';
import { RouterLink } from '@angular/router';

@Component({
  selector: 'docs-not-found-page',
  imports: [RouterLink],
  template: `
    <div class="mx-auto max-w-3xl px-4 py-24 text-center sm:px-6">
      <p class="font-mono text-6xl font-bold text-ink-600">404</p>
      <h1 class="mt-4 text-2xl font-bold text-white">Page not found</h1>
      <p class="mt-3 text-mist-400">The page you are looking for does not exist or has moved.</p>
      <a
        routerLink="/"
        class="mt-8 inline-block rounded-md bg-accent-500 px-5 py-2.5 text-sm font-semibold text-ink-950 hover:bg-accent-400"
      >Back to documentation</a>
    </div>
  `,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class NotFoundPage {}
