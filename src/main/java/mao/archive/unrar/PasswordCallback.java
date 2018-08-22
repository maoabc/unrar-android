package mao.archive.unrar;

import java.io.IOException;

public abstract class PasswordCallback implements UnrarCallback{
    @Override
    public final boolean processData(byte[] b, int off, int len) throws IOException {
        throw new IOException("Not support");
    }

}
