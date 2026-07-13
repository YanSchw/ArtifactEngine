import { Directive, HostListener } from '@angular/core';

/**
 * Handles clicks on the `.copy-btn` elements the MarkdownService injects into rendered
 * code blocks. Applied to any container that binds such HTML via `[innerHTML]`, since the
 * buttons live outside Angular's template and need delegated handling.
 */
@Directive({
  selector: '[docsCopyCode]',
})
export class CopyCodeDirective {
  @HostListener('click', ['$event'])
  protected onClick(event: MouseEvent): void {
    const button = (event.target as HTMLElement).closest<HTMLButtonElement>('.copy-btn');
    if (!button) return;

    const code = button.parentElement?.querySelector('pre code')?.textContent ?? '';
    if (this.copy(code)) {
      button.classList.add('copied');
      setTimeout(() => button.classList.remove('copied'), 1100);
    }
  }

  /**
   * Copies synchronously so it stays inside the click's user-activation window. The
   * execCommand path is tried first because it works in insecure contexts (file://, plain
   * http) and never loses the gesture; the async Clipboard API is a best-effort fallback.
   */
  private copy(text: string): boolean {
    const textarea = document.createElement('textarea');
    textarea.value = text;
    textarea.setAttribute('readonly', '');
    textarea.style.position = 'fixed';
    textarea.style.top = '0';
    textarea.style.left = '0';
    textarea.style.opacity = '0';
    document.body.appendChild(textarea);
    textarea.select();
    textarea.setSelectionRange(0, text.length);
    let ok = false;
    try {
      ok = document.execCommand('copy');
    } catch {
      ok = false;
    }
    document.body.removeChild(textarea);

    if (!ok && navigator.clipboard?.writeText) {
      navigator.clipboard.writeText(text).catch(() => {});
      return true;
    }
    return ok;
  }
}
