import { Injectable, computed } from '@angular/core';
import { httpResource } from '@angular/common/http';
import { ApiDocs, ApiType, GuideEntry } from './api.models';

@Injectable({ providedIn: 'root' })
export class ApiDocsService {
  private readonly api = httpResource<ApiDocs>(() => 'generated/api.json');
  private readonly guidesManifest = httpResource<GuideEntry[]>(() => 'guides/manifest.json');

  readonly loading = computed(() => this.api.isLoading());
  readonly error = computed(() => this.api.error());

  readonly engineVersion = computed(() => this.api.value()?.engineVersion ?? '');
  readonly modules = computed(() => this.api.value()?.modules ?? []);
  readonly types = computed(() => this.api.value()?.types ?? []);
  readonly guides = computed(() => this.guidesManifest.value() ?? []);

  private readonly typesByName = computed(() => {
    const map = new Map<string, ApiType>();
    for (const type of this.types()) {
      map.set(type.name, type);
    }
    return map;
  });

  private readonly derivedByParent = computed(() => {
    const map = new Map<string, ApiType[]>();
    for (const type of this.types()) {
      if (!type.parent) continue;
      const siblings = map.get(type.parent) ?? [];
      siblings.push(type);
      map.set(type.parent, siblings);
    }
    return map;
  });

  getType(name: string): ApiType | undefined {
    return this.typesByName().get(name);
  }

  getModule(name: string) {
    return this.modules().find((m) => m.name === name);
  }

  getTypesOfModule(moduleName: string): ApiType[] {
    return this.types().filter((t) => t.module === moduleName);
  }

  getDerivedTypes(name: string): ApiType[] {
    return this.derivedByParent().get(name) ?? [];
  }

  /** Parent chain from the immediate parent up to the root (parents outside the reflection data included by name). */
  getInheritanceChain(type: ApiType): { name: string; known: boolean }[] {
    const chain: { name: string; known: boolean }[] = [];
    let parent = type.parent;
    const guard = new Set<string>([type.name]);
    while (parent && !guard.has(parent)) {
      guard.add(parent);
      const known = this.typesByName().has(parent);
      chain.push({ name: parent, known });
      parent = known ? this.typesByName().get(parent)!.parent : undefined;
    }
    return chain;
  }
}
