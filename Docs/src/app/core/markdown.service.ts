import { Injectable, inject } from '@angular/core';
import { DomSanitizer, SafeHtml } from '@angular/platform-browser';
import { marked } from 'marked';
import hljs from 'highlight.js/lib/core';
import cpp from 'highlight.js/lib/languages/cpp';
import bash from 'highlight.js/lib/languages/bash';
import json from 'highlight.js/lib/languages/json';
import python from 'highlight.js/lib/languages/python';
import cmake from 'highlight.js/lib/languages/cmake';

hljs.registerLanguage('cpp', cpp);
hljs.registerLanguage('bash', bash);
hljs.registerLanguage('sh', bash);
hljs.registerLanguage('json', json);
hljs.registerLanguage('python', python);
hljs.registerLanguage('cmake', cmake);

export interface TocEntry {
  id: string;
  text: string;
  level: number;
}

export interface RenderedMarkdown {
  html: SafeHtml;
  toc: TocEntry[];
}

const COPY_ICON =
  '<svg class="copy-icon-copy" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2"/><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"/></svg>';
const CHECK_ICON =
  '<svg class="copy-icon-check" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="m20 6-11 11-5-5"/></svg>';

@Injectable({ providedIn: 'root' })
export class MarkdownService {
  private readonly sanitizer = inject(DomSanitizer);

  /** Renders trusted repo markdown to HTML with heading anchors, a TOC and highlighted code blocks. */
  render(markdown: string): RenderedMarkdown {
    const container = document.createElement('div');
    container.innerHTML = marked.parse(markdown, { async: false });

    const toc: TocEntry[] = [];
    const usedIds = new Set<string>();
    for (const heading of container.querySelectorAll<HTMLElement>('h1, h2, h3, h4')) {
      const text = heading.textContent ?? '';
      let id = this.slugify(text);
      let n = 2;
      while (usedIds.has(id)) id = `${this.slugify(text)}-${n++}`;
      usedIds.add(id);
      heading.id = id;

      const anchor = document.createElement('a');
      // Includes the current path so the link stays an in-page jump despite <base href="/">.
      anchor.href = `${window.location.pathname}#${id}`;
      anchor.className = 'heading-anchor';
      anchor.textContent = '#';
      anchor.setAttribute('aria-label', `Link to ${text}`);
      heading.appendChild(anchor);

      const level = Number(heading.tagName[1]);
      if (level >= 2 && level <= 3) {
        toc.push({ id, text, level });
      }
    }

    for (const block of container.querySelectorAll<HTMLElement>('pre code')) {
      const language = [...block.classList]
        .find((c) => c.startsWith('language-'))
        ?.slice('language-'.length);
      if (language && hljs.getLanguage(language)) {
        block.innerHTML = hljs.highlight(block.textContent ?? '', { language }).value;
      }
    }

    for (const pre of container.querySelectorAll<HTMLElement>('pre')) {
      const wrapper = document.createElement('div');
      wrapper.className = 'code-block';
      pre.parentNode?.insertBefore(wrapper, pre);
      wrapper.appendChild(pre);

      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'copy-btn';
      button.setAttribute('aria-label', 'Copy code');
      button.innerHTML = COPY_ICON + CHECK_ICON;
      wrapper.appendChild(button);
    }

    return {
      html: this.sanitizer.bypassSecurityTrustHtml(container.innerHTML),
      toc,
    };
  }

  highlightCpp(code: string): SafeHtml {
    return this.sanitizer.bypassSecurityTrustHtml(hljs.highlight(code, { language: 'cpp' }).value);
  }

  private slugify(text: string): string {
    return text
      .toLowerCase()
      .trim()
      .replace(/[^\w\s-]/g, '')
      .replace(/\s+/g, '-');
  }
}
