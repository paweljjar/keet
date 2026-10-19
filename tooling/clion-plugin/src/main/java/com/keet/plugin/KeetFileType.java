package com.keet.plugin;

import com.intellij.openapi.fileTypes.LanguageFileType;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import javax.swing.Icon;

public class KeetFileType extends LanguageFileType {
    public static final KeetFileType INSTANCE = new KeetFileType();

    private KeetFileType() {
        super(KeetLanguage.INSTANCE);
    }

    @NotNull
    @Override
    public String getName() {
        return "Keet";
    }

    @NotNull
    @Override
    public String getDescription() {
        return "KeetScript Single-File Component";
    }

    @NotNull
    @Override
    public String getDefaultExtension() {
        return "keet";
    }

    @Nullable
    @Override
    public Icon getIcon() {
        return KeetIcons.FILE;
    }
}
