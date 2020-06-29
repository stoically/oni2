open EditorCoreTypes;

open Oni_Core;

open Helpers;

module Diagnostic = Feature_LanguageSupport.Diagnostic;
module Definition = Feature_LanguageSupport.Definition;

let renderLine =
    (
      ~context,
      ~buffer,
      ~colors: Colors.t,
      ~diagnosticsMap,
      ~selectionRanges,
      ~matchingPairs,
      ~bufferHighlights,
      item,
      _offset,
    ) => {
  let index = Index.fromZeroBased(item);
  let renderDiagnostics = (colors: Colors.t, diagnostic: Diagnostic.t) =>
    Draw.underline(~context, ~color=colors.errorForeground, diagnostic.range);

  /* Draw error markers */
  switch (IntMap.find_opt(item, diagnosticsMap)) {
  | None => ()
  | Some(diagnostics) => List.iter(renderDiagnostics(colors), diagnostics)
  };

  switch (Hashtbl.find_opt(selectionRanges, index)) {
  | None => ()
  | Some(selections) =>
    List.iter(
      Draw.range(~context, ~color=colors.selectionBackground),
      selections,
    )
  };

  /* Draw match highlights */
  switch (matchingPairs) {
  | None => ()
  | Some((startPos, endPos)) =>
    Draw.range(
      ~context,
      ~color=colors.selectionBackground,
      Range.{start: startPos, stop: startPos},
    );
    Draw.range(
      ~context,
      ~color=colors.selectionBackground,
      Range.{start: endPos, stop: endPos},
    );
  };

  /* Draw search highlights */
  BufferHighlights.getHighlightsByLine(
    ~bufferId=Buffer.getId(buffer),
    ~line=index,
    bufferHighlights,
  )
  |> List.iter(
       Draw.range(~context, ~padding=1., ~color=colors.findMatchBackground),
     );
};

let renderEmbellishments =
    (
      ~context,
      ~count,
      ~buffer,
      ~colors,
      ~diagnosticsMap,
      ~selectionRanges,
      ~matchingPairs,
      ~bufferHighlights,
    ) =>
  Draw.renderImmediate(
    ~context,
    ~count,
    renderLine(
      ~context,
      ~buffer,
      ~colors,
      ~diagnosticsMap,
      ~selectionRanges,
      ~matchingPairs,
      ~bufferHighlights,
    ),
  );

let renderDefinition =
    (
      ~context,
      ~leftVisibleColumn,
      ~cursorPosition: Location.t,
      ~editor,
      ~bufferHighlights,
      ~colors,
      ~matchingPairs,
      ~bufferSyntaxHighlights,
      ~bufferWidthInCharacters,
    ) =>
  getTokenAtPosition(
    ~editor,
    ~bufferHighlights,
    ~cursorLine=Index.toZeroBased(cursorPosition.line),
    ~colors,
    ~matchingPairs,
    ~bufferSyntaxHighlights,
    ~startIndex=leftVisibleColumn,
    ~endIndex=leftVisibleColumn + bufferWidthInCharacters,
    cursorPosition,
  )
  |> Option.iter((token: BufferViewTokenizer.t) => {
       let range =
         Range.{
           start:
             Location.{
               line: cursorPosition.line,
               column: token.startPosition,
             },
           stop:
             Location.{line: cursorPosition.line, column: token.endPosition},
         };
       Draw.underline(~context, ~color=token.color, range);
     });

let renderTokens =
    (~context, ~line, ~colors, ~tokens, ~shouldRenderWhitespace) => {
  tokens
  |> WhitespaceTokenFilter.filter(shouldRenderWhitespace)
  |> List.iter(Draw.token(~context, ~line, ~colors));
};

let renderText =
    (
      ~context,
      ~count,
      ~selectionRanges,
      ~editor,
      ~bufferHighlights,
      ~cursorLine,
      ~colors,
      ~matchingPairs,
      ~bufferSyntaxHighlights,
      ~leftVisibleColumn,
      ~shouldRenderWhitespace,
      ~bufferWidthInCharacters,
    ) =>
  Draw.renderImmediate(
    ~context,
    ~count,
    (item, _offsetY) => {
      let index = Index.fromZeroBased(item);
      let selectionRange =
        switch (Hashtbl.find_opt(selectionRanges, index)) {
        | None => None
        | Some(v) =>
          switch (List.length(v)) {
          | 0 => None
          | _ => Some(List.hd(v))
          }
        };
      let tokens =
        getTokensForLine(
          ~editor,
          ~bufferHighlights,
          ~cursorLine,
          ~colors,
          ~matchingPairs,
          ~bufferSyntaxHighlights,
          ~selection=selectionRange,
          leftVisibleColumn,
          leftVisibleColumn + bufferWidthInCharacters,
          item,
        );

      renderTokens(
        ~context,
        ~line=item,
        ~colors,
        ~tokens,
        ~shouldRenderWhitespace,
      );
    },
  );

let render =
    (
      ~context,
      ~count,
      ~buffer,
      ~editor,
      ~leftVisibleColumn,
      ~colors,
      ~diagnosticsMap,
      ~selectionRanges,
      ~matchingPairs,
      ~bufferHighlights,
      ~cursorPosition: Location.t,
      ~definition,
      ~bufferSyntaxHighlights,
      ~shouldRenderWhitespace,
      ~bufferWidthInCharacters,
    ) => {
  renderEmbellishments(
    ~context,
    ~count,
    ~buffer,
    ~colors,
    ~diagnosticsMap,
    ~selectionRanges,
    ~matchingPairs,
    ~bufferHighlights,
  );

  if (Definition.isAvailable(
        Buffer.getId(buffer),
        cursorPosition,
        definition,
      )) {
    renderDefinition(
      ~context,
      ~editor,
      ~leftVisibleColumn,
      ~cursorPosition,
      ~bufferHighlights,
      ~colors,
      ~matchingPairs,
      ~bufferSyntaxHighlights,
      ~bufferWidthInCharacters,
    );
  };

  renderText(
    ~context,
    ~count,
    ~selectionRanges,
    ~editor,
    ~bufferHighlights,
    ~cursorLine=Index.toZeroBased(cursorPosition.line),
    ~colors,
    ~matchingPairs,
    ~bufferSyntaxHighlights,
    ~leftVisibleColumn,
    ~shouldRenderWhitespace,
    ~bufferWidthInCharacters,
  );
};
