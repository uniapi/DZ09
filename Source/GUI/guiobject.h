#ifndef _GUIOBJECT_H_
#define _GUIOBJECT_H_

typedef enum tag_GOTYPE
{
    GO_UNKNOWN,
    GO_WINDOW,

    GO_NUMTYPES
} TGOTYPE;

typedef struct tag_GUIHEADER *pGUIHEADER;
typedef struct tag_GUIHEADER
{
    TRECT      Position;
    pGUIHEADER Parent;
    TGOTYPE    Type;
    boolean    Enabled;
    boolean    Visible;
    int32_t    Tag;
    void       (*OnPressed)(pGUIHEADER, pPOINT);
    void       (*OnReleased)(pGUIHEADER, pPOINT);
    void       (*OnMove)(pGUIHEADER, pPOINT);
    void       (*OnClick)(pGUIHEADER, pPOINT);
    void       (*OnPaint)(pGUIHEADER, pRECT);
} TGUIHEADER, *pGUIHEADER;

typedef struct tag_WIN *pWIN;
typedef struct tag_WIN
{
    TSYSOBJECT  Type;
    TGUIHEADER  Head;
    boolean     Topmost;
    uint32_t    ForeColor;
    uint32_t    BackColor;
    TDLIST      ChildObjects;
    boolean     (*EventHandler)(pEVENT, pWIN);
} TWIN, *pWIN;

#endif /* _GUIOBJECT_H_ */