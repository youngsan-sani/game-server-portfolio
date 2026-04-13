void CTradeDirector::finishTrade(CPlayer* player, CPlayer* targetPlayer)
{
	// 롤백 함수는 나중에 등록된 것 부터 진행하도록 stack으로 등록
	rollbackFunctionStack.clear();

	// 양측의 거래 목록 아이템 삭제, 대상의 아이템이 들어올 수 있는지는 삭제 후 검사해야하므로 먼저 삭제하고 실패 시 롤백
	auto [tradeItemList, targetTradeItemList]= getTradeItemList(player, targetPlayer);
	if (deleteItemList(player, tradeItemList) != SUCCESS)
	{
		// 등록된 롤백함수는 없지만 코드 일관성을 위해 추가
		rollbackFunctionStack.rollback();
		onFinishTrade(player, targetPlayer, TRADE_LIST_DELETE_FAIL);
		return;
	}
	rollbackFunctionStack.push([=]() { rollbackDeleteItemList(player, tradeItemList); });

	if (deleteItemList(targetPlayer, targetTradeItemList) != SUCCESS)
	{
		rollbackFunctionStack.rollback();
		onFinishTrade(player, targetPlayer, TRADE_LIST_DELETE_FAIL);
		return;
	}
	rollbackFunctionStack.push([=]() { rollbackDeleteItemList(targetPlayer, targetTradeItemList); });

	// 대상의 거래 목록이 요청자의 인벤토리에 들어올 수 있는지 확인
	if (canAddNewItemList(player, targetTradeItemList) != SUCCESS)
	{
		rollbackFunctionStack.rollback();
		onFinishTrade(player, targetPlayer, INVENTORY_FULL);
		return;
	}

	// 내 거래 목록이 대상의 인벤토리에 들어갈 수 있는지 확인
	if (canAddNewItemList(targetPlayer, tradeItemList) != CS_ERROR_NONE)
	{
		rollbackFunctionStack.rollback();
		onFinishTrade(player, targetPlayer, TARGET_INVENTORY_FULL);
		return;
	}

	// 양측의 골드 차감
	auto [tradeGold, targetTradeGold] = getTradeGold(player, targetPlayer);
	if (useGold(player, tradeGold) != SUCCESS)
	{
		rollbackFunctionStack.rollback();
		onFinishTrade(player, targetPlayer, NOT_ENOUGH_GOLD);
		return;
	}
	rollbackFunctionStack.push([=]() { rollbackUseGold(player, tradeGold); });

	if (useGold(targetPlayer, targetTradeGold) != SUCCESS)
	{
		rollbackFunctionStack.rollback();
		onFinishTrade(player, targetPlayer, TARGET_NOT_ENOUGH_GOLD);
		return;
	}
	rollbackFunctionStack.push([=]() { rollbackUseGold(targetPlayer, targetTradeGold); });

	// 거래 목록을 각각의 인벤토리에 추가
	if (addItemList(player, targetTradeItemList) != SUCCESS)
	{
		rollbackFunctionStack.rollback();
		onFinishTrade(player, targetPlayer, TRADE_LIST_ADD_FAIL);
		return;
	}
	rollbackFunctionStack.push([=]() { rollbackAddItemList(player, targetTradeItemList); });

	if (addItemList(targetPlayer, tradeItemList) != SUCCESS)
	{
		rollbackFunctionStack.rollback();
		onFinishTrade(player, targetPlayer, TRADE_LIST_ADD_FAIL);
		return;
	}
	// 이후 실패가 없어 등록하지 않아도 되지만 코드 일관성을 위해 추가
	rollbackFunctionStack.push([=]() { rollbackAddItemList(targetPlayer, tradeItemList); });

	// 골드 추가는 항상 성공
	addGold(player, targetTradeGold);
	addGold(targetPlayer, tradeGold);

	// 등록된 롤백 함수 초기화
	rollbackFunctionStack.clear();

	// 거래 성공 처리 및 거래 상태 초기화
	onFinishTrade(player, targetPlayer, SUCCESS);
}
