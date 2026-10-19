package com.keet.plugin;

import com.intellij.codeInsight.completion.*;
import com.intellij.codeInsight.lookup.LookupElementBuilder;
import com.intellij.patterns.PlatformPatterns;
import com.intellij.util.ProcessingContext;
import org.jetbrains.annotations.NotNull;

public class KeetCompletionContributor extends CompletionContributor {
    public KeetCompletionContributor() {
        extend(CompletionType.BASIC, PlatformPatterns.psiElement(), new CompletionProvider<>() {
            @Override
            protected void addCompletions(@NotNull CompletionParameters parameters,
                                          @NotNull ProcessingContext context,
                                          @NotNull CompletionResultSet result) {
                result.addElement(LookupElementBuilder.create("bind:value={}")
                        .withPresentableText("bind:value")
                        .withTailText("={stateVar} (Two-way text/number input sync)")
                        .withTypeText("Keet Directive", true));

                result.addElement(LookupElementBuilder.create("bind:checked={}")
                        .withPresentableText("bind:checked")
                        .withTailText("={boolVar} (Two-way checkbox sync)")
                        .withTypeText("Keet Directive", true));

                String[][] events = {
                        {"onClick", "Click event callback"},
                        {"onInput", "Input text change callback"},
                        {"onChange", "Selection or value commit callback"},
                        {"onSubmit", "Form submission callback"},
                        {"onKeyDown", "Keyboard key down callback"},
                        {"onKeyUp", "Keyboard key up callback"},
                        {"onFocus", "Element focus callback"},
                        {"onBlur", "Element blur callback"}
                };

                for (String[] evt : events) {
                    result.addElement(LookupElementBuilder.create(evt[0] + "={() => }")
                            .withPresentableText(evt[0])
                            .withTailText("={() => ...} (" + evt[1] + ")")
                            .withTypeText("Keet Event", true));
                }

                result.addElement(LookupElementBuilder.create("state count: int = 0;")
                        .withPresentableText("state")
                        .withTailText(" name: type = init; (Reactive component state)")
                        .withTypeText("KeetScript Keyword", true));

                result.addElement(LookupElementBuilder.create("let title: string = \"\";")
                        .withPresentableText("let")
                        .withTailText(" name: type = default; (Component prop or constant)")
                        .withTypeText("KeetScript Keyword", true));

                result.addElement(LookupElementBuilder.create("def handleEvent() {\n    \n}")
                        .withPresentableText("def")
                        .withTailText(" name(params) -> returnType { ... }")
                        .withTypeText("KeetScript Function", true));

                result.addElement(LookupElementBuilder.create("import cpp from \"Header.h\" as Module;")
                        .withPresentableText("import cpp from")
                        .withTailText(" \"Header.h\" as Module; (Native C++ bridge)")
                        .withTypeText("Keet Bridge", true));

                result.addElement(LookupElementBuilder.create("<script>\n    \n</script>")
                        .withPresentableText("<script>")
                        .withTypeText("SFC Section", true));

                result.addElement(LookupElementBuilder.create("<template>\n    \n</template>")
                        .withPresentableText("<template>")
                        .withTypeText("SFC Section", true));

                result.addElement(LookupElementBuilder.create("<style>\n    \n</style>")
                        .withPresentableText("<style>")
                        .withTypeText("SFC Section", true));
            }
        });
    }
}
