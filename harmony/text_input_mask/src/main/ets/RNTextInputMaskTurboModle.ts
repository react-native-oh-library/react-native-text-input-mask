/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

import { TurboModule, TurboModuleContext } from '@rnoh/react-native-openharmony/ts';

export class RNTextInputMaskTurboModule extends TurboModule  {
  private context: TurboModuleContext;

  constructor(ctx: TurboModuleContext) {
    super(ctx)
    this.context = ctx
  }

  mask(mask: string, value: string, autocomplete: boolean): Promise<string> {
    return;
  }

  unmask(mask: string, value: string, autocomplete: boolean): Promise<string> {
    return;
  }

  setMask(reactNode: number, primaryFormat: string, options: object): void {
    console.log("==================", "setMask")
  }

}