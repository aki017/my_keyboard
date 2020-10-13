import React, { useState } from "react";
import { KeyCode } from "./Consts";

export const KeyBindDialog = (props: {
  value: KeyCode;
  setValue: (value: KeyCode) => void;
}) => {
  const [text, setText] = useState(props.value);

  return (
    <div>
      <input
        type="text"
        value={text}
        onChange={(e) => setText(e.target.value as KeyCode)}
      ></input>
      <button onClick={() => props.setValue(text)}>Save</button>
    </div>
  );
};
