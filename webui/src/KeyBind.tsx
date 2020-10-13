import React, { useState } from "react";
import { keyCode, KeyCode } from "./Consts";
import { KeyBindDialog } from "./KeyBindDialog";

const Button = ({
  x = 0,
  y = 0,
  xscale = 1,
  yscale = 1,
  rotate = 0,
  code = "NULL",
  onClick = () => {},
}) => (
  <>
    <rect
      x={x - (30 * xscale) / 2}
      y={y - (30 * yscale) / 2}
      width={30 * xscale}
      height={30 * yscale}
      rx="5"
      ry="5"
      fill="black"
      transform={`rotate(${rotate || 0},${x},${y})`}
      onClick={onClick}
    />
    <text
      x={x}
      y={y}
      dominantBaseline="middle"
      textAnchor="middle"
      fill="white"
      transform={`rotate(${rotate || 0},${x},${y})`}
      fontSize={code.length <= 2 ? 16 : code.length <= 6 ? 8 : 6}
      pointerEvents="none"
    >
      {code}
    </text>
  </>
);

export const KeyBind = () => {
  const [binding, setBinding] = useState([-1, 0]);
  const [map, setMap] = useState<KeyCode[]>([
    "KC_ESCAPE",
    "KC_1",
    "KC_2",
    "KC_3",
    "KC_4",
    "KC_5",
    "KC_GRAVE",
    "KC_Q",
    "KC_W",
    "KC_E",
    "KC_R",
    "KC_T",
    "KC_TAB",
    "KC_A",
    "KC_S",
    "KC_D",
    "KC_F",
    "KC_G",
    "KC_LSHIFT",
    "KC_Z",
    "KC_X",
    "KC_C",
    "KC_V",
    "KC_B",
    "KC_NO",
    "KC_NO",
    "KC_TAB",
    "KC_BSLASH",
    "KC_DELETE",
    "KC_LSHIFT",
    "KC_SPACE",
    "KC_LCTRL",
    "KC_ENTER",
    "KC_LALT",
    "KC_NO",
    "KC_NO",
  ]);
  const [map2, setMap2] = useState<KeyCode[]>([
    "KC_6",
    "KC_7",
    "KC_8",
    "KC_9",
    "KC_0",
    "KC_MINUS",
    "KC_Y",
    "KC_U",
    "KC_I",
    "KC_O",
    "KC_P",
    "KC_EQUAL",
    "KC_H",
    "KC_J",
    "KC_K",
    "KC_L",
    "KC_SCOLON",
    "KC_QUOTE",
    "KC_N",
    "KC_M",
    "KC_COMMA",
    "KC_DOT",
    "KC_SLASH",
    "KC_RSHIFT",
    "KC_SPACE",
    "KC_BSPACE",
    "KC_LBRACKET",
    "KC_RBRACKET",
    "",
    "",
    "KC_RCTRL",
    "KC_RALT",
    "KC_RGUI",
    "KC_PGDOWN",
    "",
    "",
  ] as KeyCode[]);

  const kb = (
    x: number,
    y: number,
    flip: boolean,
    lr: 0 | 1,
    code: number,
    xscale?: number,
    yscale?: number,
    rotate = 0
  ) => {
    return (
      <Button
        x={flip ? 260 - x : x}
        y={y}
        xscale={xscale}
        yscale={yscale}
        rotate={rotate * (flip ? 1 : -1)}
        code={keyCode[[map, map2][lr][code]]?.label }
        onClick={() => setBinding([lr, code])}
      />
    );
  };

  return (
    <div>
      {binding[0] === -1 ? null : (
        <KeyBindDialog
          value={(binding[0] === 0 ? map : map2)[binding[1]]}
          setValue={(value) => {
            const data : KeyCode[] = [...[map, map2][binding[0]]];
            data[binding[1]] = value;
            (binding[0] === 0 ? setMap : setMap2)(data);
            setBinding([-1, 0]);
          }}
        />
      )}
      <svg
        width="520"
        height="640"
        viewBox="0, 0, 260, 320"
        xmlns="http://www.w3.org/2000/svg"
      >
        {kb(50, 40, true, 0, 5)}
        {kb(85, 40, true, 0, 4)}
        {kb(120, 35, true, 0, 3)}
        {kb(155, 35, true, 0, 2)}
        {kb(190, 45, true, 0, 1)}
        {kb(225, 45, true, 0, 0)}

        {kb(50, 75, true, 0, 11)}
        {kb(85, 75, true, 0, 10)}
        {kb(120, 70, true, 0, 9)}
        {kb(155, 70, true, 0, 8)}
        {kb(190, 80, true, 0, 7)}
        {kb(225, 80, true, 0, 6)}

        {kb(50, 110, true, 0, 17)}
        {kb(85, 110, true, 0, 16)}
        {kb(120, 105, true, 0, 15)}
        {kb(155, 105, true, 0, 14)}
        {kb(190, 115, true, 0, 13)}
        {kb(225, 115, true, 0, 12)}

        {kb(50, 145, true, 0, 23)}
        {kb(85, 145, true, 0, 22)}
        {kb(120, 140, true, 0, 21)}
        {kb(155, 140, true, 0, 20)}
        {kb(190, 150, true, 0, 19)}
        {kb(225, 150, true, 0, 18)}

        {kb(120, 175, true, 0, 27)}
        {kb(155, 175, true, 0, 26)}

        {kb(40, 210, true, 0, 29, 1, 1.5, 20)}
        {kb(70, 195, true, 0, 28, 1, 1.5, 20)}

        {kb(40, 255, true, 0, 31, 1, 1, 80)}
        {kb(37, 290, true, 0, 33, 1, 1, 80)}

        {kb(72, 262, true, 0, 30, 1, 1, 80)}
        {kb(70, 295, true, 0, 32, 1, 1, 80)}
      </svg>
      <svg
        width="520"
        height="640"
        viewBox="0, 0, 260, 320"
        xmlns="http://www.w3.org/2000/svg"
      >
        {kb(50, 40, false, 1, 0)}
        {kb(85, 40, false, 1, 1)}
        {kb(120, 35, false, 1, 2)}
        {kb(155, 35, false, 1, 3)}
        {kb(190, 45, false, 1, 4)}
        {kb(225, 45, false, 1, 5)}

        {kb(50, 75, false, 1, 6)}
        {kb(85, 75, false, 1, 7)}
        {kb(120, 70, false, 1, 8)}
        {kb(155, 70, false, 1, 9)}
        {kb(190, 80, false, 1, 10)}
        {kb(225, 80, false, 1, 11)}

        {kb(50, 110, false, 1, 12)}
        {kb(85, 110, false, 1, 13)}
        {kb(120, 105, false, 1, 14)}
        {kb(155, 105, false, 1, 15)}
        {kb(190, 115, false, 1, 16)}
        {kb(225, 115, false, 1, 17)}

        {kb(50, 145, false, 1, 18)}
        {kb(85, 145, false, 1, 19)}
        {kb(120, 140, false, 1, 20)}
        {kb(155, 140, false, 1, 21)}
        {kb(190, 150, false, 1, 22)}
        {kb(225, 150, false, 1, 23)}

        {kb(120, 175, false, 1, 26)}
        {kb(155, 175, false, 1, 27)}

        {kb(40, 210, false, 1, 24, 1, 1.5, 20)}
        {kb(70, 195, false, 1, 25, 1, 1.5, 20)}

        {kb(40, 255, false, 1, 30, 1, 1, 80)}
        {kb(37, 290, false, 1, 31, 1, 1, 80)}

        {kb(72, 262, false, 1, 33, 1, 1, 80)}
        {kb(70, 295, false, 1, 32, 1, 1, 80)}
      </svg>

      <pre>
        uint8_t input_map[] = {`{\n`}
        {map.map((s, i) => `${s},` + (i % 6 === 5 ? "\n" : "")).join("")}
        {`}`};
      </pre>
      <pre>
        uint8_t input_map[] = {`{\n`}
        {map2.map((s, i) => `${s}, ` + (i % 6 === 5 ? "\n" : "")).join("")}
        {`}`};
      </pre>
    </div>
  );
};
