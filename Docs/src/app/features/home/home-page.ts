import { ChangeDetectionStrategy, Component, computed, inject } from '@angular/core';
import { RouterLink } from '@angular/router';
import { ApiDocsService } from '../../core/api-docs.service';

@Component({
  selector: 'docs-home-page',
  imports: [RouterLink],
  templateUrl: './home-page.html',
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class HomePage {
  protected readonly docs = inject(ApiDocsService);

  protected readonly classCount = computed(
    () => this.docs.types().filter((t) => t.kind === 'class').length,
  );
  protected readonly structCount = computed(
    () => this.docs.types().filter((t) => t.kind === 'struct').length,
  );
  protected readonly enumCount = computed(
    () => this.docs.types().filter((t) => t.kind === 'enum').length,
  );
}
