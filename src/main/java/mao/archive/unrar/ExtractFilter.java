package mao.archive.unrar;

import androidx.annotation.Keep;

/**
 * 批量解压过滤
 * Created by mao on 17-6-7.
 */
@Keep
public interface ExtractFilter {
    boolean accept(RarEntry entry);
}
