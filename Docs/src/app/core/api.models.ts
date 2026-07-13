export type TypeKind = 'class' | 'struct' | 'enum';

export interface ApiProperty {
  type: string;
  name: string;
  default?: string;
  description?: string;
}

export interface ApiEnumValue {
  name: string;
  value: number | string;
  description?: string;
}

export interface ApiType {
  kind: TypeKind;
  name: string;
  module: string;
  header: string;
  parent?: string;
  description?: string;
  properties?: ApiProperty[];
  isEnumClass?: boolean;
  underlyingType?: string;
  values?: ApiEnumValue[];
}

export interface ApiModule {
  name: string;
  importModules: string[];
  types: string[];
  mountContentDir?: string;
  supportedPlatforms?: string[];
}

export interface ApiDocs {
  engineVersion: string;
  generatedAt: string;
  modules: ApiModule[];
  types: ApiType[];
}

export interface GuideEntry {
  slug: string;
  title: string;
  summary: string;
}
