import { Routes } from '@angular/router';

export const routes: Routes = [
  {
    path: '',
    loadComponent: () => import('./features/home/home-page').then((m) => m.HomePage),
    title: 'Artifact Engine Documentation',
  },
  {
    path: 'guides/:slug',
    loadComponent: () => import('./features/guides/guide-page').then((m) => m.GuidePage),
  },
  {
    path: 'api',
    loadComponent: () => import('./features/api/api-index-page').then((m) => m.ApiIndexPage),
    title: 'API Reference — Artifact Engine',
  },
  {
    path: 'api/modules/:name',
    loadComponent: () => import('./features/api/module-page').then((m) => m.ModulePage),
  },
  {
    path: 'api/types/:name',
    loadComponent: () => import('./features/api/type-page').then((m) => m.TypePage),
  },
  {
    path: '**',
    loadComponent: () => import('./features/not-found-page').then((m) => m.NotFoundPage),
    title: 'Page not found — Artifact Engine',
  },
];
