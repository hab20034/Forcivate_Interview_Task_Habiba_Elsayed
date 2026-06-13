import json
import os
from pathlib import Path

MODEL_PATH = Path(__file__).parent / "models" / "model.gguf"
OPENAI_MODEL = os.environ.get("OPENAI_MODEL", "gpt-3.5-turbo")
OPENAI_TEMPERATURE = float(os.environ.get("OPENAI_TEMPERATURE", 0.7))
LLM_MAX_TOKENS = int(os.environ.get("LLM_MAX_TOKENS", 250))

try:
    from llama_cpp import Llama
    if MODEL_PATH.exists():
        llm = Llama(model_path=str(MODEL_PATH), n_ctx=2048)
    else:
        llm = None
        print(f"Local model file not found: {MODEL_PATH}", flush=True)
except Exception as exc:
    llm = None
    print(f"Local llama_cpp import failed: {exc}", flush=True)

try:
    import openai
except ImportError:
    openai = None


def load_examples():
    examples_path = Path(__file__).parent / "learning_examples.json"
    if not examples_path.exists():
        return []

    try:
        with open(examples_path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return []


def build_prompt(post, comment, tone, use_emoji, length):
    use_emoji_text = "yes" if use_emoji else "no"
    examples = load_examples()
    few_shot = ""

    for ex in examples[-3:]:
        few_shot += (
            "### EXAMPLE START\n"
            f"Comment: {ex['comment']}\n"
            f"Reply: {ex['reply']}\n"
            "### EXAMPLE END\n\n"
        )

    prompt = (
        "You are a social media assistant."
        "\nThe user will provide a post and a comment."
        "\nYour job is to write a single short reply to the comment."
        "\nRespond with the reply text only, without quoting the post or comment."
        f"\nTone: {tone}"
        f"\nUse Emoji: {use_emoji_text}"
        f"\nLength: {length}"
        "\n\n"
    )

    if few_shot:
        prompt += "Approved reply examples for style reference only:\n\n" + few_shot

    prompt += (
        f"Post: {post}\n"
        f"Comment: {comment}\n"
        "Reply:"
    )

    return prompt


def normalize_response(response):
    if isinstance(response, dict):
        choices = response.get("choices") or []
        if choices:
            first = choices[0]
            if isinstance(first, dict):
                return first.get("text", "").strip()
            return str(first).strip()
        return str(response.get("text", "")).strip()

    if hasattr(response, "choices") and response.choices:
        choice = response.choices[0]
        if hasattr(choice, "text"):
            return choice.text.strip()
        if hasattr(choice, "message") and hasattr(choice.message, "content"):
            return choice.message.content.strip()
        return str(choice).strip()

    if hasattr(response, "text"):
        return response.text.strip()

    return str(response).strip()


def generate_local_reply(prompt):
    if llm is None:
        raise RuntimeError("Local llama_cpp model is not available")

    stop_sequences = ["\nPost:", "\nComment:", "\nReply:", "\n###"]

    if hasattr(llm, "create_completion"):
        response = llm.create_completion(
            prompt=prompt,
            max_tokens=LLM_MAX_TOKENS,
            temperature=OPENAI_TEMPERATURE,
            stop=stop_sequences,
        )
    elif hasattr(llm, "create"):
        response = llm.create(
            prompt=prompt,
            max_tokens=LLM_MAX_TOKENS,
            temperature=OPENAI_TEMPERATURE,
            stop=stop_sequences,
        )
    elif hasattr(llm, "generate"):
        response = llm.generate(
            prompt=prompt,
            max_tokens=LLM_MAX_TOKENS,
            temperature=OPENAI_TEMPERATURE,
            stop=stop_sequences,
        )
    else:
        response = llm(
            prompt=prompt,
            max_tokens=LLM_MAX_TOKENS,
            temperature=OPENAI_TEMPERATURE,
        )

    return normalize_response(response)


def generate_openai_reply(prompt):
    if openai is None:
        raise RuntimeError("OpenAI package is not installed")

    if hasattr(openai, "ChatCompletion"):
        response = openai.ChatCompletion.create(
            model=OPENAI_MODEL,
            messages=[
                {"role": "system", "content": "You are a helpful social media assistant."},
                {"role": "user", "content": prompt},
            ],
            max_tokens=LLM_MAX_TOKENS,
            temperature=OPENAI_TEMPERATURE,
            stop=["\nPost:", "\nComment:", "\nReply:"],
        )
        if response.choices and response.choices[0].message:
            return response.choices[0].message.content.strip()
        return normalize_response(response)

    response = openai.Completion.create(
        model=OPENAI_MODEL,
        prompt=prompt,
        max_tokens=LLM_MAX_TOKENS,
        temperature=OPENAI_TEMPERATURE,
        stop=["\nPost:", "\nComment:", "\nReply:"],
    )
    return normalize_response(response)


def generate_reply(post, comment, tone, use_emoji, length):
    prompt = build_prompt(post, comment, tone, use_emoji, length)

    if llm is not None:
        try:
            return generate_local_reply(prompt)
        except Exception as exc:
            print(f"Local model generation failed: {exc}", flush=True)

    return generate_openai_reply(prompt)
