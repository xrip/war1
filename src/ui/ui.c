#include "ui.h"

#include <assert.h>
#include <stdlib.h>

#include "shl/wstr.h"

#include "audio/audio.h"
#include "units/entities.h"
#include "data/resources.h"

bool wui_isUIEntity(WarEntity* entity)
{
    switch (entity->type)
    {
        case WAR_ENTITY_TYPE_IMAGE:
        case WAR_ENTITY_TYPE_TEXT:
        case WAR_ENTITY_TYPE_RECT:
        case WAR_ENTITY_TYPE_BUTTON:
        case WAR_ENTITY_TYPE_CURSOR:
        case WAR_ENTITY_TYPE_MINIMAP:
            return true;

        default:
            return false;
    }
}

void wui_clearUIText(WarContext* context, WarEntity* uiText)
{
    WarTextComponent* text = we_getTextComponent(context, uiText);
    assert(text);

    wstr_free(text->text);
    text->text = wstr_make();

    we_disableComponent(context, uiText, COMP_TEXT);
}

void wui_clearUITooltip(WarContext* context, WarEntity* uiButton)
{
    WarButtonComponent* button = we_getButtonComponent(context, uiButton);
    assert(button);

    wstr_free(button->tooltip);
    button->tooltip = wstr_make();
}

void wui_changeCursorType(WarContext* context, WarCursorType type)
{
    context->imui.cursor_type = type;
}

void wui_updateUIButtons(WarContext* context, bool hotKeysEnabled)
{
    WarInput* input = &context->input;
    WarEntityList* buttons = we_getEntitiesOfType(context, WAR_ENTITY_TYPE_BUTTON);
    WarEntity* hoveredButton = NULL;
    WarEntity* capturedButton = NULL;

    // NOTE: Store the buttons to update in this frame first
    // because the action of some buttons is to show other buttons
    // in their same location, and if the newly shown button is
    // after in the list, then it will update in this same frame
    // which shouldn't happen
    WarEntityIdSet buttonsToUpdate;
    WarEntityIdSetInit(&buttonsToUpdate, wm_frameAllocator(), we_hashEntityId, we_equalsEntityId);

    for(s32 i = 0; i < buttons->count; i++)
    {
        WarEntity* entity = buttons->items[i];
        if (entity)
        {
            WarButtonComponent* button = we_getButtonComponent(context, entity);
            assert(button);

            if (we_isComponentEnabled(context, entity, COMP_UI) && we_isComponentEnabled(context, entity, COMP_BUTTON) && button->interactive)
            {
                WarEntityIdSetAdd(&buttonsToUpdate, entity->id);

                WarTransformComponent* transform = we_getTransformComponent(context, entity);
                assert(transform);

                vec2 backgroundSize = vec2i(button->normalSprite.frameWidth, button->normalSprite.frameHeight);
                rect buttonRect = rectv(transform->position, backgroundSize);
                if (rect_containsf(buttonRect, input->pos.x, input->pos.y))
                {
                    hoveredButton = entity;
                }
            }
            else
            {
                button->hot = false;
                button->active = false;

                if (input->capturedUIButtonId == entity->id)
                {
                    input->capturedUIButtonId = 0;
                }
            }
        }
    }

    if (input->capturedUIButtonId)
    {
        capturedButton = we_findEntity(context, input->capturedUIButtonId);
        if (!capturedButton || !WarEntityIdSetContains(&buttonsToUpdate, capturedButton->id))
        {
            input->capturedUIButtonId = 0;
            capturedButton = NULL;
        }
    }

    if (isButtonJustPressed(input, WAR_MOUSE_LEFT) && hoveredButton)
    {
        input->capturedUIButtonId = hoveredButton->id;
        capturedButton = hoveredButton;
    }

    for(s32 i = 0; i < buttons->count; i++)
    {
        WarEntity* entity = buttons->items[i];
        if (entity && WarEntityIdSetContains(&buttonsToUpdate, entity->id))
        {
            WarButtonComponent* button = we_getButtonComponent(context, entity);
            assert(button);

            bool isHovered = entity == hoveredButton;
            bool isCaptured = entity == capturedButton;

            button->hot = isHovered;
            button->active = isHovered && isCaptured && isButtonHeld(input, WAR_MOUSE_LEFT);

            if (hotKeysEnabled && isKeyJustReleased(input, button->hotKey))
            {
                if (button->clickHandler)
                {
                    button->hot = false;
                    button->active = false;

                    button->clickHandler(context, entity);

                    // in this case break to not allow pressing multiple keys
                    // and executing all of the command for those keys
                    break;
                }
            }

            if (isButtonJustReleased(input, WAR_MOUSE_LEFT) && isCaptured)
            {
                if (isHovered && button->clickHandler)
                {
                    button->clickHandler(context, entity);
                    wa_createAudio(context, CREATE_AUDIO_ARGS_INIT(.audioId=WAR_UI_CLICK, .loop=false));
                }

                button->active = false;
                input->capturedUIButtonId = 0;
            }
        }
    }

    WarEntityIdSetFree(&buttonsToUpdate);
}

void wui_renderUIEntities(WarContext* context)
{
    WarEntityList* entities = we_getUIEntities(context);
    for(s32 i = 0; i < entities->count; i++)
    {
        WarEntity *entity = entities->items[i];
        // Cursor entities are now rendered by imui_end(); skip them here.
        if (entity && entity->type != WAR_ENTITY_TYPE_CURSOR)
        {
            we_renderEntity(context, entity);
        }
    }
}
