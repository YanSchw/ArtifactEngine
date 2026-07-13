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
