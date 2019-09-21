package com.github.maoabc.unrar;

import java.io.IOException;

public interface OnCreateOutCallback {
    UnrarCallback createOut(String entryName) throws IOException;
}
