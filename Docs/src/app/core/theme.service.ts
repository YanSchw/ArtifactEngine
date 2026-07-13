import { Injectable, signal } from '@angular/core';

export type ThemePreference = 'light' | 'dark' | 'system';

const STORAGE_KEY = 'theme';

@Injectable({ providedIn: 'root' })
export class ThemeService {
  private readonly systemLight = window.matchMedia('(prefers-color-scheme: light)');
  private readonly _preference = signal<ThemePreference>(readStoredPreference());
  readonly preference = this._preference.asReadonly();

  constructor() {
    this.systemLight.addEventListener('change', () => {
      if (this._preference() === 'system') this.applyResolved();
    });
    this.applyResolved();
  }

  set(preference: ThemePreference): void {
    this._preference.set(preference);
    if (preference === 'system') localStorage.removeItem(STORAGE_KEY);
    else localStorage.setItem(STORAGE_KEY, preference);
    this.applyResolved();
  }

  cycle(): void {
    const order: ThemePreference[] = ['system', 'light', 'dark'];
    this.set(order[(order.indexOf(this._preference()) + 1) % order.length]);
  }

  private applyResolved(): void {
    const preference = this._preference();
    const resolved = preference === 'system' ? (this.systemLight.matches ? 'light' : 'dark') : preference;
    document.documentElement.dataset['theme'] = resolved;
  }
}

function readStoredPreference(): ThemePreference {
  const stored = localStorage.getItem(STORAGE_KEY);
  return stored === 'light' || stored === 'dark' ? stored : 'system';
}
