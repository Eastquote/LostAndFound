// WARNING: This is an internal implementation header, which must be included from a specific location/namespace
//          That is the reason that this header does not contain a #pragma once, nor namespace guards

// Helper struct representing a transition event to a new FSM state
struct TransitionEvent
{
	Task<> newTask;
	StateId newStateId;
};

// Base class for defining links between states
class LinkBase
{
public:
	virtual ~LinkBase() = default;
	virtual std::optional<TransitionEvent> EvaluateLink() const = 0;
};

// Type-safe link handle
class LinkHandle
{
	bool IsOnCompleteLink() const
	{
		return m_linkType == eType::OnComplete;
	}
	bool HasCondition() const
	{
		return m_isConditionalLink;
	}

protected:
	// Link-type enum
	enum class eType
	{
		Normal,
		OnComplete,
	};

	// Friends
	template<class, class> friend class StateHandle;
	friend class ::TaskFSM;

	// Constructors (friend-only)
	LinkHandle() = delete;
	LinkHandle(std::shared_ptr<LinkBase> in_link, eType in_linkType, bool in_isConditional)
		: m_link(std::move(in_link))
		, m_linkType(in_linkType)
		, m_isConditionalLink(in_isConditional)
	{
	}
	std::optional<TransitionEvent> EvaluateLink() const
	{
		return m_link->EvaluateLink();
	}

private:
	std::shared_ptr<LinkBase> m_link; // The underlying link
	eType m_linkType; // Whether the link is normal or OnComplete
	bool m_isConditionalLink; // Whether the link has an associated condition predicate
};

// Internal FSM state object
template<class tStateInput, class tStateConstructorFn>
struct State
{
	State(tStateConstructorFn in_stateCtorFn, StateId in_stateId, std::string in_debugName)
		: m_stateCtorFn(in_stateCtorFn)
		, m_stateId(in_stateId)
		, m_debugName(in_debugName)
	{
	}

	tStateConstructorFn m_stateCtorFn;
	StateId m_stateId;
	std::string m_debugName;
};

// Internal FSM state object (exit state specialization)
template<>
struct State<void, void>
{
	State(StateId in_stateId, std::string in_debugName)
		: m_stateId(in_stateId)
		, m_debugName(in_debugName)
	{
	}

	StateId m_stateId;
	std::string m_debugName;
};

// Internal link definition object
template<class ReturnT, class tStateConstructorFn, class tPredicateFn>
class Link : public LinkBase
{
public:
	Link(std::shared_ptr<State<ReturnT, tStateConstructorFn>> in_targetState, tPredicateFn in_predicate)
	: m_targetState(std::move(in_targetState))
	, m_predicate(in_predicate)
	{
	}

private:
	virtual std::optional<TransitionEvent> EvaluateLink() const final
	{
		if(std::optional<ReturnT> result = m_predicate())
		{
			return TransitionEvent{ m_targetState->m_stateCtorFn(result.value()), m_targetState->m_stateId };
		}
		return std::optional<TransitionEvent>();
	}

	std::shared_ptr<State<ReturnT, tStateConstructorFn>> m_targetState;
	tPredicateFn m_predicate;
};

// Internal link definition object (no-payload specialization)
template<class tStateConstructorFn, class tPredicateFn>
class Link<void, tStateConstructorFn, tPredicateFn> : public LinkBase
{
public:
	Link(std::shared_ptr<State<void, tStateConstructorFn>> in_targetState, tPredicateFn in_predicate)
		: m_targetState(std::move(in_targetState))
		, m_predicate(in_predicate)
	{
	}

private:
	virtual std::optional<TransitionEvent> EvaluateLink() const final
	{
		return m_predicate() ? TransitionEvent{ m_targetState->m_stateCtorFn(), m_targetState->m_stateId } : std::optional<TransitionEvent>{};
	}

	std::shared_ptr<State<void, tStateConstructorFn>> m_targetState;
	tPredicateFn m_predicate;
};

// Internal link definition object (exit-state specialization)
template<class tPredicateFn>
class Link<void, void, tPredicateFn> : public LinkBase
{
public:
	Link(std::shared_ptr<State<void, void>> in_targetState, tPredicateFn in_predicate)
		: m_targetState(std::move(in_targetState))
		, m_predicate(in_predicate)
	{
	}

private:
	virtual std::optional<TransitionEvent> EvaluateLink() const final
	{
		return m_predicate() ? TransitionEvent{ Task<>(), m_targetState->m_stateId } : std::optional<TransitionEvent>{};
	}

	std::shared_ptr<State<void, void>> m_targetState;
	tPredicateFn m_predicate;
};

// Helper template to extract the first argument of a function (or void if it has no arguments) at compile-time
template<typename Ret> void first_argument_helper(Ret(*) ());
template<typename Ret, typename Arg, typename... Rest> Arg first_argument_helper(Ret(*) (Arg, Rest...));
template <typename F> decltype(first_argument_helper(&F::operator())) first_argument_helper(F);
template <typename T> using first_argument = decltype(first_argument_helper(std::declval<T>()));
